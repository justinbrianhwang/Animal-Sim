#include "simulation/Animal.h"
#include "simulation/AnimalRegistry.h"
#include "simulation/DrugCompound.h"
#include "simulation/DrugRegistry.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace animsim {

Animal::Animal(int id, Species species, int groupIndex, RandomEngine& rng)
    : m_id(id), m_species(species), m_groupIndex(groupIndex)
{
    const auto& params = AnimalRegistry::getParams(species);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%s-%03d", params.name, id);
    m_label = buf;

    initializePhysiology(rng);
}

void Animal::initializePhysiology(RandomEngine& rng) {
    const auto& sp = AnimalRegistry::getParams(m_species);

    // Individual variation
    m_sensitivityFactor = std::max(0.3f, rng.normal(1.0f, 0.15f));
    m_metabolismFactor = std::max(0.5f, rng.normal(1.0f, 0.1f));

    // Vitals with individual variation
    m_state.vitals.weight = std::max(0.01f, rng.normal(sp.weightMean, sp.weightStd));
    m_state.vitals.heartRate = std::max(30.0f, rng.normal(sp.heartRateMean, sp.heartRateStd));
    m_state.vitals.temperature = rng.normal(sp.tempMean, sp.tempStd);
    m_state.vitals.respiratoryRate = std::max(5.0f, rng.normal(sp.respRateMean, sp.respRateStd));
    m_state.vitals.bloodPressureSys = rng.normal(120.0f, 8.0f);
    m_state.vitals.bloodPressureDia = rng.normal(80.0f, 5.0f);
    m_state.vitals.oxygenSaturation = std::min(100.0f, rng.normal(98.5f, 0.5f));

    // Blood chemistry with slight variation
    m_state.blood.wbc = rng.normal(8.0f, 1.0f);
    m_state.blood.rbc = rng.normal(8.5f, 0.5f);
    m_state.blood.platelets = rng.normal(800.0f, 80.0f);
    m_state.blood.hemoglobin = rng.normal(15.0f, 1.0f);
    m_state.blood.alt = rng.normal(45.0f, 8.0f);
    m_state.blood.ast = rng.normal(120.0f, 15.0f);
    m_state.blood.bun = rng.normal(20.0f, 3.0f);
    m_state.blood.creatinine = rng.normal(0.5f, 0.05f);
    m_state.blood.glucose = rng.normal(120.0f, 10.0f);
    m_state.blood.albumin = rng.normal(3.5f, 0.3f);

    // Organs start at 100%
    m_state.organs = OrganHealth{};
    m_state.status = AnimalStatus::Alive;
    m_state.overallHealth = 100.0f;
    m_state.drugConcentration = 0.0f;
    m_state.cumulativeDose = 0.0f;
    m_state.activeDrugs.clear();
    m_state.timeOfDeath = -1.0f;
}

void Animal::administerDose(float doseMgKg, const std::string& route,
                             const std::string& drugId) {
    if (!isAlive()) return;

    auto& drug = DrugRegistry::instance().getDrug(drugId);
    float bioavailability = drug.getBioavailability(route);
    float effectiveDose = doseMgKg * bioavailability * m_sensitivityFactor;

    // Find existing entry for this drug or create new
    bool found = false;
    for (auto& ad : m_state.activeDrugs) {
        if (ad.drugId == drugId) {
            ad.concentration += effectiveDose;
            ad.cumulativeDose += doseMgKg;
            found = true;
            break;
        }
    }
    if (!found) {
        ActiveDrug ad;
        ad.drugId = drugId;
        ad.concentration = effectiveDose;
        ad.cumulativeDose = doseMgKg;
        ad.timeAdministered = 0.0f; // set externally if needed
        m_state.activeDrugs.push_back(ad);
    }

    // Update computed totals
    recomputeTotals();
}

void Animal::applySalineFlush() {
    if (!isAlive()) return;
    // Reduces all drug concentrations by 30%
    for (auto& ad : m_state.activeDrugs) {
        ad.concentration *= 0.7f;
    }
    recomputeTotals();
}

void Animal::applyActivatedCharcoal() {
    if (!isAlive()) return;
    // Reduces all drug concentrations by 50% (simulates reduced absorption)
    for (auto& ad : m_state.activeDrugs) {
        ad.concentration *= 0.5f;
    }
    recomputeTotals();
}

void Animal::applyAntidote(const std::string& targetDrugId) {
    if (!isAlive()) return;
    // Remove specific drug almost entirely (90% reduction)
    for (auto& ad : m_state.activeDrugs) {
        if (ad.drugId == targetDrugId) {
            ad.concentration *= 0.1f;
            break;
        }
    }
    recomputeTotals();
}

void Animal::recomputeTotals() {
    m_state.drugConcentration = 0.0f;
    m_state.cumulativeDose = 0.0f;
    for (auto& ad : m_state.activeDrugs) {
        m_state.drugConcentration += ad.concentration;
        m_state.cumulativeDose += ad.cumulativeDose;
    }
}

void Animal::updatePhysiology(float dt, RandomEngine& rng) {
    if (!isAlive()) return;

    const auto& sp = AnimalRegistry::getParams(m_species);

    // Per-drug elimination using drug-specific half-life
    for (auto& ad : m_state.activeDrugs) {
        auto& drug = DrugRegistry::instance().getDrug(ad.drugId);
        float halfLife = std::max(0.01f, drug.halfLifeHours);
        float eliminationRate = 0.693f / halfLife * m_metabolismFactor * sp.metabolicRate;
        ad.concentration *= std::exp(-eliminationRate * dt);
    }

    // Remove negligible drug entries
    m_state.activeDrugs.erase(
        std::remove_if(m_state.activeDrugs.begin(), m_state.activeDrugs.end(),
                       [](const ActiveDrug& ad) { return ad.concentration < 0.01f; }),
        m_state.activeDrugs.end());

    recomputeTotals();

    // Vital signs respond to total drug concentration
    float drugEffect = m_state.drugConcentration / 50.0f; // normalized
    drugEffect = std::min(drugEffect, 3.0f);

    // Heart rate: increases then decreases at high doses
    float hrBase = sp.heartRateMean;
    if (drugEffect < 1.0f) {
        m_state.vitals.heartRate = hrBase * (1.0f + 0.15f * drugEffect) + rng.normal(0, sp.heartRateStd * 0.1f);
    } else {
        m_state.vitals.heartRate = hrBase * (1.15f - 0.3f * (drugEffect - 1.0f)) + rng.normal(0, sp.heartRateStd * 0.1f);
    }
    m_state.vitals.heartRate = std::max(20.0f, m_state.vitals.heartRate);

    // Temperature: slight increase with drug
    m_state.vitals.temperature = sp.tempMean + 0.5f * drugEffect + rng.normal(0, 0.1f);

    // Respiratory rate: increases with drug
    m_state.vitals.respiratoryRate = sp.respRateMean * (1.0f + 0.2f * drugEffect) + rng.normal(0, sp.respRateStd * 0.1f);
    m_state.vitals.respiratoryRate = std::max(5.0f, m_state.vitals.respiratoryRate);

    // O2 saturation drops at high concentrations
    m_state.vitals.oxygenSaturation = std::min(100.0f, 98.5f - 5.0f * std::max(0.0f, drugEffect - 1.0f));

    // Blood chemistry changes
    m_state.blood.alt = 45.0f + 50.0f * drugEffect;
    m_state.blood.ast = 120.0f + 80.0f * drugEffect;
    m_state.blood.bun = 20.0f + 15.0f * std::max(0.0f, drugEffect - 0.5f);
    m_state.blood.creatinine = 0.5f + 0.3f * std::max(0.0f, drugEffect - 0.5f);
    m_state.blood.wbc = 8.0f + 4.0f * drugEffect;

    // Weight loss at high doses
    if (drugEffect > 0.5f) {
        m_state.vitals.weight -= 0.001f * drugEffect * dt;
        m_state.vitals.weight = std::max(m_state.vitals.weight * 0.7f, m_state.vitals.weight);
    }

    // Overall health from organs
    m_state.overallHealth = computeHealthFromOrgans();

    // Status update
    if (m_state.overallHealth < 30.0f && m_state.status == AnimalStatus::Alive) {
        m_state.status = AnimalStatus::Moribund;
    }
}

void Animal::applyToxicDamage(float severity, RandomEngine& rng) {
    if (!isAlive()) return;

    // Use drug-specific damage for each active drug
    if (m_state.activeDrugs.empty()) {
        // Fallback: generic damage (legacy behavior)
        applyDrugSpecificDamage("generic_toxicant", severity, rng);
    } else {
        // Drug interaction bonus: 20% more damage if 2+ drugs active
        float interactionBonus = m_state.activeDrugs.size() >= 2 ? 1.2f : 1.0f;

        for (auto& ad : m_state.activeDrugs) {
            // Weight severity by this drug's contribution to total concentration
            float fraction = (m_state.drugConcentration > 0.01f)
                ? ad.concentration / m_state.drugConcentration
                : 1.0f / m_state.activeDrugs.size();
            float drugSeverity = severity * fraction * interactionBonus;
            applyDrugSpecificDamage(ad.drugId, drugSeverity, rng);
        }
    }
}

void Animal::applyDrugSpecificDamage(const std::string& drugId, float severity, RandomEngine& rng) {
    auto& drug = DrugRegistry::instance().getDrug(drugId);

    severity *= m_sensitivityFactor;

    // Organ damage weighted by drug's toxicity profile
    float baseDamage = 10.0f; // max damage per tick
    m_state.organs.liver    -= severity * drug.hepatotoxicity * rng.uniform(3.0f, baseDamage);
    m_state.organs.kidney   -= severity * drug.nephrotoxicity * rng.uniform(3.0f, baseDamage);
    m_state.organs.heart    -= severity * drug.cardiotoxicity * rng.uniform(2.0f, baseDamage * 0.8f);
    m_state.organs.brain    -= severity * drug.neurotoxicity * rng.uniform(2.0f, baseDamage * 0.8f);
    m_state.organs.lungs    -= severity * drug.pulmonaryToxicity * rng.uniform(2.0f, baseDamage * 0.7f);
    m_state.organs.stomach  -= severity * drug.giToxicity * rng.uniform(2.0f, baseDamage * 0.6f);
    m_state.organs.intestines -= severity * drug.giToxicity * rng.uniform(1.0f, baseDamage * 0.5f);
    m_state.organs.bone_marrow -= severity * drug.hematotoxicity * rng.uniform(2.0f, baseDamage * 0.7f);
    m_state.organs.spleen   -= severity * drug.hematotoxicity * rng.uniform(1.0f, baseDamage * 0.5f);
    m_state.organs.skin     -= severity * drug.dermalToxicity * rng.uniform(1.0f, baseDamage * 0.4f);

    // Clamp all organs to [0, 100]
    auto clamp = [](float& v) { v = std::max(0.0f, std::min(100.0f, v)); };
    clamp(m_state.organs.liver);
    clamp(m_state.organs.kidney);
    clamp(m_state.organs.heart);
    clamp(m_state.organs.lungs);
    clamp(m_state.organs.brain);
    clamp(m_state.organs.stomach);
    clamp(m_state.organs.intestines);
    clamp(m_state.organs.skin);
    clamp(m_state.organs.spleen);
    clamp(m_state.organs.bone_marrow);
}

void Animal::checkMortality(RandomEngine& rng) {
    if (!isAlive()) return;

    float health = computeHealthFromOrgans();

    // Critical organ failure
    if (m_state.organs.heart < 10.0f || m_state.organs.brain < 10.0f ||
        m_state.organs.lungs < 15.0f) {
        kill(-1.0f); // time set externally
        return;
    }

    // Probabilistic death based on health
    if (health < 20.0f) {
        float deathProb = (20.0f - health) / 20.0f * 0.3f;
        if (rng.chance(deathProb)) {
            kill(-1.0f);
        }
    }
}

void Animal::kill(float timeHours) {
    m_state.status = AnimalStatus::Dead;
    if (timeHours >= 0.0f) m_state.timeOfDeath = timeHours;
}

void Animal::euthanize(float timeHours) {
    m_state.status = AnimalStatus::Euthanized;
    m_state.timeOfDeath = timeHours;
}

float Animal::computeHealthFromOrgans() const {
    // Weighted average - critical organs count more
    float h = m_state.organs.heart * 0.18f +
              m_state.organs.brain * 0.18f +
              m_state.organs.lungs * 0.15f +
              m_state.organs.liver * 0.15f +
              m_state.organs.kidney * 0.12f +
              m_state.organs.stomach * 0.06f +
              m_state.organs.intestines * 0.06f +
              m_state.organs.bone_marrow * 0.05f +
              m_state.organs.spleen * 0.03f +
              m_state.organs.skin * 0.02f;
    return h;
}

} // namespace animsim
