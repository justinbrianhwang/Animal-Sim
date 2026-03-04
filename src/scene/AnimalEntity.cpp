#include "scene/AnimalEntity.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace animsim {

AnimalEntity::AnimalEntity() {}

void AnimalEntity::buildModel(Species species) {
    m_species = species;
    m_parts.clear();

    switch (species) {
    case Species::Mouse:    buildMouse(); break;
    case Species::Rabbit:   buildRabbit(); break;
    case Species::GuineaPig: buildGuineaPig(); break;
    case Species::Dog:      buildDog(); break;
    case Species::Monkey:   buildMonkey(); break;
    default:                buildRat(); break;
    }

    updateTransforms();
}

void AnimalEntity::addPart(Mesh&& mesh, glm::vec3 offset, glm::vec3 scale,
                            glm::vec3 color, float spec) {
    Part p;
    p.mesh = std::move(mesh);
    p.localOffset = offset;
    p.localScale = scale;
    p.baseColor = color;
    p.currentColor = color;
    p.specular = spec;
    m_parts.push_back(std::move(p));
}

// ─── RAT ──────────────────────────────────────────────────────────────────

void AnimalEntity::buildRat() {
    m_scale = 1.0f;
    glm::vec3 fur(0.75f, 0.68f, 0.60f);
    glm::vec3 belly(0.88f, 0.83f, 0.78f);
    glm::vec3 ear(0.92f, 0.72f, 0.70f);
    glm::vec3 nose(0.90f, 0.65f, 0.62f);
    glm::vec3 eye(0.08f, 0.06f, 0.06f);
    glm::vec3 tail(0.85f, 0.72f, 0.68f);
    glm::vec3 paw(0.88f, 0.75f, 0.72f);

    // Body (elongated, low to surface)
    addPart(Mesh::createCapsule(0.07f, 0.12f, 16, glm::vec3(1)),
            {0.0f, 0.0f, 0.0f}, {1.0f, 0.85f, 0.9f}, fur, 0.15f);
    addPart(Mesh::createSphere(0.055f, 12, glm::vec3(1)),
            {0.0f, -0.025f, 0.0f}, {1.5f, 0.6f, 0.85f}, belly, 0.1f);
    addPart(Mesh::createSphere(0.06f, 12, glm::vec3(1)),
            {-0.06f, 0.0f, 0.0f}, {1.1f, 0.95f, 1.05f}, fur, 0.15f);
    // Head (pointed)
    addPart(Mesh::createSphere(0.05f, 14, glm::vec3(1)),
            {0.13f, 0.015f, 0.0f}, {1.3f, 0.95f, 0.95f}, fur, 0.15f);
    addPart(Mesh::createSphere(0.025f, 10, glm::vec3(1)),
            {0.20f, 0.005f, 0.0f}, {1.6f, 0.8f, 0.85f}, fur * 0.95f, 0.15f);
    addPart(Mesh::createSphere(0.012f, 8, glm::vec3(1)),
            {0.235f, 0.008f, 0.0f}, {1.0f, 0.8f, 0.9f}, nose, 0.6f);
    // Whisker pads
    for (float z : {0.018f, -0.018f})
        addPart(Mesh::createSphere(0.015f, 8, glm::vec3(1)),
                {0.21f, 0.0f, z}, {1.2f, 0.8f, 1.0f}, fur * 0.97f, 0.1f);
    // Small round ears
    for (float z : {0.028f, -0.028f}) {
        addPart(Mesh::createSphere(0.022f, 10, glm::vec3(1)),
                {0.10f, 0.065f, z}, {0.5f, 1.0f, 0.9f}, ear, 0.1f);
        addPart(Mesh::createSphere(0.015f, 8, glm::vec3(1)),
                {0.10f, 0.067f, z * 0.9f}, {0.4f, 0.85f, 0.75f}, ear * 0.85f, 0.05f);
    }
    // Eyes (small, dark)
    for (float z : {0.025f, -0.025f}) {
        addPart(Mesh::createSphere(0.009f, 8, glm::vec3(1)),
                {0.17f, 0.035f, z}, {1.0f, 1.0f, 1.0f}, eye, 0.95f);
        addPart(Mesh::createSphere(0.004f, 6, glm::vec3(1)),
                {0.175f, 0.04f, z * 0.8f}, {1.0f, 1.0f, 1.0f}, glm::vec3(0.9f), 1.0f);
    }
    // Long thin tail
    addPart(Mesh::createTaperedCylinder(0.007f, 0.003f, 0.18f, 8, glm::vec3(1)),
            {-0.2f, -0.01f, 0.0f}, {1.0f, 1.0f, 1.0f}, tail, 0.2f);
    // Short legs
    for (float z : {0.035f, -0.035f}) {
        addPart(Mesh::createTaperedCylinder(0.013f, 0.01f, 0.035f, 8, glm::vec3(1)),
                {0.06f, -0.055f, z}, {1.0f, 1.0f, 1.0f}, fur * 0.92f, 0.1f);
        addPart(Mesh::createSphere(0.01f, 6, glm::vec3(1)),
                {0.06f, -0.075f, z}, {1.3f, 0.6f, 1.0f}, paw, 0.1f);
    }
    for (float z : {0.04f, -0.04f}) {
        addPart(Mesh::createTaperedCylinder(0.016f, 0.012f, 0.04f, 8, glm::vec3(1)),
                {-0.06f, -0.05f, z}, {1.0f, 1.0f, 1.0f}, fur * 0.92f, 0.1f);
        addPart(Mesh::createSphere(0.013f, 6, glm::vec3(1)),
                {-0.06f, -0.075f, z}, {1.5f, 0.6f, 1.0f}, paw, 0.1f);
    }
}

// ─── MOUSE (tiny, white, BIG round ears) ──────────────────────────────────

void AnimalEntity::buildMouse() {
    m_scale = 0.55f;
    glm::vec3 fur(0.95f, 0.93f, 0.90f);
    glm::vec3 ear(0.95f, 0.70f, 0.68f);
    glm::vec3 nose(0.92f, 0.68f, 0.65f);
    glm::vec3 eye(0.05f, 0.05f, 0.05f);
    glm::vec3 tail(0.88f, 0.72f, 0.70f);
    glm::vec3 paw(0.92f, 0.78f, 0.75f);

    // Small body
    addPart(Mesh::createCapsule(0.065f, 0.08f, 14, glm::vec3(1)),
            {0.0f, 0.0f, 0.0f}, {1.0f, 0.85f, 0.9f}, fur, 0.15f);
    addPart(Mesh::createSphere(0.055f, 10, glm::vec3(1)),
            {-0.04f, -0.005f, 0.0f}, {1.0f, 0.9f, 1.0f}, fur, 0.15f);
    // Head
    addPart(Mesh::createSphere(0.048f, 12, glm::vec3(1)),
            {0.10f, 0.015f, 0.0f}, {1.15f, 1.0f, 1.0f}, fur, 0.15f);
    addPart(Mesh::createSphere(0.02f, 8, glm::vec3(1)),
            {0.165f, 0.005f, 0.0f}, {1.4f, 0.7f, 0.8f}, fur, 0.15f);
    addPart(Mesh::createSphere(0.01f, 6, glm::vec3(1)),
            {0.20f, 0.008f, 0.0f}, {1.0f, 0.8f, 0.8f}, nose, 0.5f);
    // HUGE round ears (proportionally much bigger than rat)
    for (float z : {0.035f, -0.035f}) {
        addPart(Mesh::createSphere(0.035f, 12, glm::vec3(1)),
                {0.08f, 0.07f, z}, {0.35f, 1.0f, 0.95f}, ear, 0.1f);
        addPart(Mesh::createSphere(0.025f, 10, glm::vec3(1)),
                {0.08f, 0.072f, z * 0.85f}, {0.3f, 0.85f, 0.8f}, ear * 0.82f, 0.05f);
    }
    // Eyes (beady)
    for (float z : {0.024f, -0.024f}) {
        addPart(Mesh::createSphere(0.01f, 8, glm::vec3(1)),
                {0.14f, 0.035f, z}, {1.0f, 1.0f, 1.0f}, eye, 0.95f);
        addPart(Mesh::createSphere(0.005f, 6, glm::vec3(1)),
                {0.145f, 0.04f, z * 0.75f}, {1.0f, 1.0f, 1.0f}, glm::vec3(0.85f), 1.0f);
    }
    // Long thin tail
    addPart(Mesh::createTaperedCylinder(0.005f, 0.002f, 0.2f, 6, glm::vec3(1)),
            {-0.18f, -0.01f, 0.0f}, {1.0f, 1.0f, 1.0f}, tail, 0.2f);
    // Tiny legs
    for (float z : {0.03f, -0.03f}) {
        addPart(Mesh::createTaperedCylinder(0.01f, 0.008f, 0.03f, 6, glm::vec3(1)),
                {0.04f, -0.05f, z}, {1.0f, 1.0f, 1.0f}, fur * 0.92f, 0.1f);
        addPart(Mesh::createSphere(0.008f, 6, glm::vec3(1)),
                {0.04f, -0.068f, z}, {1.2f, 0.5f, 1.0f}, paw, 0.1f);
    }
    for (float z : {0.035f, -0.035f}) {
        addPart(Mesh::createTaperedCylinder(0.013f, 0.009f, 0.035f, 6, glm::vec3(1)),
                {-0.04f, -0.05f, z}, {1.0f, 1.0f, 1.0f}, fur * 0.92f, 0.1f);
        addPart(Mesh::createSphere(0.01f, 6, glm::vec3(1)),
                {-0.04f, -0.068f, z}, {1.3f, 0.5f, 1.0f}, paw, 0.1f);
    }
}

// ─── RABBIT (round body, VERY LONG upright ears, cotton tail) ─────────────

void AnimalEntity::buildRabbit() {
    m_scale = 1.6f;
    glm::vec3 fur(0.96f, 0.94f, 0.91f);
    glm::vec3 ear(0.95f, 0.78f, 0.75f);
    glm::vec3 eye(0.75f, 0.15f, 0.18f); // red eyes (albino rabbit)
    glm::vec3 nose(0.92f, 0.70f, 0.68f);
    glm::vec3 paw(0.93f, 0.90f, 0.87f);

    // Round chunky body
    addPart(Mesh::createCapsule(0.09f, 0.1f, 16, glm::vec3(1)),
            {0.0f, 0.0f, 0.0f}, {1.1f, 1.0f, 1.1f}, fur, 0.12f);
    addPart(Mesh::createSphere(0.085f, 14, glm::vec3(1)),
            {-0.06f, 0.0f, 0.0f}, {1.1f, 1.05f, 1.15f}, fur, 0.12f);
    // Chest area
    addPart(Mesh::createSphere(0.065f, 12, glm::vec3(1)),
            {0.07f, -0.01f, 0.0f}, {1.0f, 0.95f, 1.0f}, fur * 0.98f, 0.12f);
    // Head (rounder than rat)
    addPart(Mesh::createSphere(0.058f, 14, glm::vec3(1)),
            {0.13f, 0.04f, 0.0f}, {1.1f, 1.0f, 1.0f}, fur, 0.12f);
    // Snout
    addPart(Mesh::createSphere(0.03f, 10, glm::vec3(1)),
            {0.18f, 0.025f, 0.0f}, {1.2f, 0.7f, 0.9f}, fur, 0.12f);
    addPart(Mesh::createSphere(0.01f, 8, glm::vec3(1)),
            {0.21f, 0.03f, 0.0f}, {1.0f, 0.7f, 0.8f}, nose, 0.5f);
    // VERY LONG upright ears (key rabbit feature!)
    for (float z : {0.02f, -0.02f}) {
        addPart(Mesh::createCapsule(0.02f, 0.12f, 10, glm::vec3(1)),
                {0.10f, 0.14f, z}, {0.6f, 1.0f, 0.5f}, fur, 0.1f);
        addPart(Mesh::createCapsule(0.014f, 0.10f, 8, glm::vec3(1)),
                {0.10f, 0.14f, z * 0.85f}, {0.4f, 1.0f, 0.35f}, ear, 0.05f);
    }
    // Red eyes
    for (float z : {0.032f, -0.032f}) {
        addPart(Mesh::createSphere(0.013f, 10, glm::vec3(1)),
                {0.16f, 0.055f, z}, {1.0f, 1.0f, 1.0f}, eye, 0.9f);
        addPart(Mesh::createSphere(0.006f, 6, glm::vec3(1)),
                {0.165f, 0.06f, z * 0.7f}, {1.0f, 1.0f, 1.0f}, glm::vec3(0.85f), 1.0f);
    }
    // Cotton ball tail
    addPart(Mesh::createSphere(0.03f, 10, glm::vec3(1)),
            {-0.12f, 0.02f, 0.0f}, {1.0f, 1.0f, 1.0f}, fur, 0.1f);
    // Front paws
    for (float z : {0.04f, -0.04f}) {
        addPart(Mesh::createTaperedCylinder(0.018f, 0.014f, 0.05f, 8, glm::vec3(1)),
                {0.05f, -0.07f, z}, {1.0f, 1.0f, 1.0f}, fur * 0.95f, 0.1f);
        addPart(Mesh::createSphere(0.014f, 6, glm::vec3(1)),
                {0.05f, -0.098f, z}, {1.5f, 0.5f, 1.0f}, paw, 0.1f);
    }
    // Big hind legs (rabbits have powerful hind legs)
    for (float z : {0.05f, -0.05f}) {
        addPart(Mesh::createSphere(0.038f, 10, glm::vec3(1)),
                {-0.05f, -0.02f, z}, {1.3f, 1.0f, 1.0f}, fur * 0.96f, 0.12f);
        addPart(Mesh::createTaperedCylinder(0.022f, 0.016f, 0.06f, 8, glm::vec3(1)),
                {-0.05f, -0.07f, z}, {1.0f, 1.0f, 1.0f}, fur * 0.94f, 0.1f);
        addPart(Mesh::createSphere(0.02f, 6, glm::vec3(1)),
                {-0.05f, -0.105f, z}, {1.8f, 0.5f, 1.0f}, paw, 0.1f);
    }
}

// ─── GUINEA PIG (fat potato, no tail, brown+white patches) ───────────────

void AnimalEntity::buildGuineaPig() {
    m_scale = 1.3f;
    glm::vec3 brown(0.65f, 0.45f, 0.30f);
    glm::vec3 white(0.95f, 0.92f, 0.88f);
    glm::vec3 ear(0.85f, 0.65f, 0.60f);
    glm::vec3 eye(0.08f, 0.06f, 0.06f);
    glm::vec3 nose(0.75f, 0.50f, 0.45f);

    // Very round, fat potato body
    addPart(Mesh::createCapsule(0.09f, 0.05f, 16, glm::vec3(1)),
            {0.0f, 0.0f, 0.0f}, {1.1f, 0.95f, 1.15f}, brown, 0.12f);
    // White belly patch
    addPart(Mesh::createSphere(0.07f, 10, glm::vec3(1)),
            {0.0f, -0.025f, 0.0f}, {1.3f, 0.5f, 1.0f}, white, 0.1f);
    // White face patch
    addPart(Mesh::createSphere(0.04f, 10, glm::vec3(1)),
            {0.08f, 0.03f, 0.0f}, {1.0f, 1.0f, 1.0f}, white, 0.12f);
    // Head (barely distinct from body - GP feature)
    addPart(Mesh::createSphere(0.055f, 14, glm::vec3(1)),
            {0.09f, 0.01f, 0.0f}, {1.05f, 1.0f, 1.0f}, brown, 0.12f);
    addPart(Mesh::createSphere(0.025f, 8, glm::vec3(1)),
            {0.15f, 0.0f, 0.0f}, {1.2f, 0.8f, 1.0f}, brown * 0.9f, 0.15f);
    addPart(Mesh::createSphere(0.01f, 6, glm::vec3(1)),
            {0.175f, 0.005f, 0.0f}, {1.0f, 0.8f, 0.9f}, nose, 0.4f);
    // Small petal ears
    for (float z : {0.035f, -0.035f})
        addPart(Mesh::createSphere(0.02f, 8, glm::vec3(1)),
                {0.07f, 0.05f, z}, {0.5f, 0.8f, 1.0f}, ear, 0.1f);
    // Eyes
    for (float z : {0.03f, -0.03f}) {
        addPart(Mesh::createSphere(0.01f, 8, glm::vec3(1)),
                {0.12f, 0.03f, z}, {1.0f, 1.0f, 1.0f}, eye, 0.9f);
        addPart(Mesh::createSphere(0.005f, 6, glm::vec3(1)),
                {0.125f, 0.035f, z * 0.7f}, {1.0f, 1.0f, 1.0f}, glm::vec3(0.8f), 1.0f);
    }
    // NO TAIL (key guinea pig feature)
    // Very short stubby legs
    for (float z : {0.04f, -0.04f}) {
        addPart(Mesh::createTaperedCylinder(0.015f, 0.012f, 0.025f, 6, glm::vec3(1)),
                {0.04f, -0.06f, z}, {1.0f, 1.0f, 1.0f}, brown * 0.85f, 0.1f);
        addPart(Mesh::createSphere(0.012f, 6, glm::vec3(1)),
                {0.04f, -0.075f, z}, {1.3f, 0.5f, 1.0f}, brown * 0.8f, 0.1f);
    }
    for (float z : {0.042f, -0.042f}) {
        addPart(Mesh::createTaperedCylinder(0.016f, 0.013f, 0.025f, 6, glm::vec3(1)),
                {-0.04f, -0.06f, z}, {1.0f, 1.0f, 1.0f}, brown * 0.85f, 0.1f);
        addPart(Mesh::createSphere(0.013f, 6, glm::vec3(1)),
                {-0.04f, -0.075f, z}, {1.3f, 0.5f, 1.0f}, brown * 0.8f, 0.1f);
    }
}

// ─── DOG (Beagle - TALL on legs, distinct head, floppy ears) ──────────────

void AnimalEntity::buildDog() {
    m_scale = 2.5f;
    glm::vec3 brown(0.62f, 0.48f, 0.32f);
    glm::vec3 white(0.96f, 0.93f, 0.89f);
    glm::vec3 black(0.12f, 0.10f, 0.08f);
    glm::vec3 nosec(0.12f, 0.10f, 0.08f);
    glm::vec3 eye(0.15f, 0.10f, 0.06f);

    // Body elevated on legs (bodyY = how high body sits above feet)
    float by = 0.10f;

    // Torso (slimmer, elongated)
    addPart(Mesh::createCapsule(0.05f, 0.18f, 16, glm::vec3(1)),
            {0.0f, by, 0.0f}, {1.0f, 0.9f, 0.85f}, brown, 0.15f);
    // White chest/belly
    addPart(Mesh::createSphere(0.04f, 10, glm::vec3(1)),
            {0.06f, by - 0.02f, 0.0f}, {1.5f, 0.5f, 0.8f}, white, 0.12f);
    // Black saddle marking
    addPart(Mesh::createSphere(0.045f, 10, glm::vec3(1)),
            {-0.02f, by + 0.025f, 0.0f}, {1.5f, 0.4f, 0.9f}, black * 1.8f, 0.12f);

    // Neck
    addPart(Mesh::createTaperedCylinder(0.032f, 0.028f, 0.06f, 10, glm::vec3(1)),
            {0.15f, by + 0.03f, 0.0f}, {1.0f, 1.0f, 1.0f}, white * 0.95f, 0.12f);
    // Head (clearly separated from body)
    addPart(Mesh::createSphere(0.04f, 14, glm::vec3(1)),
            {0.22f, by + 0.07f, 0.0f}, {1.1f, 1.0f, 1.0f}, brown, 0.15f);
    addPart(Mesh::createSphere(0.025f, 8, glm::vec3(1)),
            {0.21f, by + 0.095f, 0.0f}, {1.3f, 0.5f, 0.8f}, brown, 0.12f);
    // Snout (long, dog-like)
    addPart(Mesh::createCapsule(0.018f, 0.05f, 10, glm::vec3(1)),
            {0.28f, by + 0.055f, 0.0f}, {1.0f, 0.8f, 0.8f}, brown * 0.95f, 0.15f);
    // Nose
    addPart(Mesh::createSphere(0.012f, 8, glm::vec3(1)),
            {0.33f, by + 0.058f, 0.0f}, {1.0f, 0.8f, 1.0f}, nosec, 0.7f);
    // Jaw
    addPart(Mesh::createSphere(0.016f, 8, glm::vec3(1)),
            {0.27f, by + 0.04f, 0.0f}, {1.2f, 0.6f, 0.8f}, white * 0.95f, 0.12f);

    // FLOPPY EARS hanging down (key beagle feature)
    for (float z : {0.030f, -0.030f}) {
        addPart(Mesh::createCapsule(0.014f, 0.06f, 8, glm::vec3(1)),
                {0.20f, by + 0.03f, z * 1.4f}, {0.7f, 1.0f, 0.6f}, brown * 0.8f, 0.1f);
    }
    // Eyes
    for (float z : {0.022f, -0.022f}) {
        addPart(Mesh::createSphere(0.01f, 10, glm::vec3(1)),
                {0.26f, by + 0.08f, z}, {1.0f, 1.0f, 1.0f}, eye, 0.9f);
        addPart(Mesh::createSphere(0.005f, 6, glm::vec3(1)),
                {0.265f, by + 0.085f, z * 0.7f}, {1.0f, 1.0f, 1.0f}, glm::vec3(0.85f), 1.0f);
    }
    // Tail (pointing up)
    addPart(Mesh::createTaperedCylinder(0.008f, 0.004f, 0.08f, 8, glm::vec3(1)),
            {-0.17f, by + 0.06f, 0.0f}, {1.0f, 1.0f, 1.0f}, white, 0.15f);

    // FRONT LEGS (tall, clearly visible - key dog feature)
    for (float z : {0.025f, -0.025f}) {
        addPart(Mesh::createTaperedCylinder(0.014f, 0.012f, 0.07f, 8, glm::vec3(1)),
                {0.10f, by - 0.055f, z}, {1.0f, 1.0f, 1.0f}, white, 0.12f);
        addPart(Mesh::createTaperedCylinder(0.012f, 0.009f, 0.06f, 8, glm::vec3(1)),
                {0.10f, by - 0.115f, z * 0.95f}, {1.0f, 1.0f, 1.0f}, white * 0.95f, 0.12f);
        addPart(Mesh::createSphere(0.011f, 6, glm::vec3(1)),
                {0.10f, by - 0.155f, z * 0.9f}, {1.4f, 0.5f, 1.0f}, white * 0.9f, 0.1f);
    }
    // HIND LEGS
    for (float z : {0.030f, -0.030f}) {
        addPart(Mesh::createSphere(0.025f, 8, glm::vec3(1)),
                {-0.09f, by - 0.01f, z}, {1.3f, 1.0f, 1.0f}, brown * 0.92f, 0.12f);
        addPart(Mesh::createTaperedCylinder(0.016f, 0.013f, 0.065f, 8, glm::vec3(1)),
                {-0.10f, by - 0.06f, z}, {1.0f, 1.0f, 1.0f}, brown, 0.12f);
        addPart(Mesh::createTaperedCylinder(0.013f, 0.010f, 0.06f, 8, glm::vec3(1)),
                {-0.10f, by - 0.115f, z * 0.95f}, {1.0f, 1.0f, 1.0f}, brown * 0.92f, 0.12f);
        addPart(Mesh::createSphere(0.012f, 6, glm::vec3(1)),
                {-0.10f, by - 0.155f, z * 0.9f}, {1.5f, 0.5f, 1.0f}, brown * 0.85f, 0.1f);
    }
}

// ─── MONKEY (upright posture, long limbs, flat face, long tail) ───────────

void AnimalEntity::buildMonkey() {
    m_scale = 2.2f;
    glm::vec3 fur(0.55f, 0.45f, 0.35f);
    glm::vec3 face(0.82f, 0.70f, 0.62f);
    glm::vec3 belly(0.70f, 0.60f, 0.50f);
    glm::vec3 eye(0.15f, 0.10f, 0.05f);
    glm::vec3 nose(0.72f, 0.55f, 0.48f);

    float by = 0.06f; // body slightly elevated

    // Torso (more upright than dog)
    addPart(Mesh::createCapsule(0.06f, 0.1f, 16, glm::vec3(1)),
            {0.0f, by, 0.0f}, {0.9f, 1.1f, 0.85f}, fur, 0.15f);
    addPart(Mesh::createSphere(0.05f, 10, glm::vec3(1)),
            {0.02f, by - 0.02f, 0.0f}, {1.1f, 0.7f, 0.9f}, belly, 0.1f);

    // Head (rounder, flatter face)
    addPart(Mesh::createSphere(0.048f, 14, glm::vec3(1)),
            {0.12f, by + 0.07f, 0.0f}, {1.0f, 1.05f, 1.0f}, fur, 0.15f);
    // Flat face area (key primate feature)
    addPart(Mesh::createSphere(0.038f, 12, glm::vec3(1)),
            {0.16f, by + 0.06f, 0.0f}, {0.9f, 0.8f, 0.85f}, face, 0.2f);
    // Brow ridge
    addPart(Mesh::createSphere(0.02f, 8, glm::vec3(1)),
            {0.15f, by + 0.09f, 0.0f}, {2.0f, 0.4f, 1.2f}, fur, 0.15f);
    // Snout/mouth area
    addPart(Mesh::createSphere(0.02f, 8, glm::vec3(1)),
            {0.19f, by + 0.05f, 0.0f}, {1.2f, 0.8f, 0.9f}, face * 0.95f, 0.2f);
    // Nose
    addPart(Mesh::createSphere(0.008f, 6, glm::vec3(1)),
            {0.215f, by + 0.055f, 0.0f}, {1.2f, 0.7f, 1.2f}, nose, 0.4f);
    // Small round ears (primate-style, on sides)
    for (float z : {0.04f, -0.04f})
        addPart(Mesh::createSphere(0.015f, 8, glm::vec3(1)),
                {0.10f, by + 0.09f, z}, {0.5f, 0.9f, 0.8f}, face * 0.9f, 0.15f);
    // Eyes (forward-facing, primate)
    for (float z : {0.018f, -0.018f}) {
        addPart(Mesh::createSphere(0.01f, 8, glm::vec3(1)),
                {0.18f, by + 0.075f, z}, {1.0f, 1.0f, 1.0f}, eye, 0.9f);
        addPart(Mesh::createSphere(0.005f, 6, glm::vec3(1)),
                {0.185f, by + 0.08f, z * 0.7f}, {1.0f, 1.0f, 1.0f}, glm::vec3(0.85f), 1.0f);
    }
    // LONG tail (key monkey feature)
    addPart(Mesh::createTaperedCylinder(0.008f, 0.003f, 0.25f, 8, glm::vec3(1)),
            {-0.22f, by + 0.01f, 0.0f}, {1.0f, 1.0f, 1.0f}, fur * 0.9f, 0.12f);

    // ARMS (longer than legs, primate feature)
    for (float z : {0.035f, -0.035f}) {
        addPart(Mesh::createTaperedCylinder(0.013f, 0.010f, 0.06f, 8, glm::vec3(1)),
                {0.06f, by - 0.05f, z}, {1.0f, 1.0f, 1.0f}, fur * 0.92f, 0.12f);
        addPart(Mesh::createTaperedCylinder(0.010f, 0.008f, 0.055f, 8, glm::vec3(1)),
                {0.06f, by - 0.10f, z * 0.9f}, {1.0f, 1.0f, 1.0f}, fur * 0.9f, 0.12f);
        // Hands (bigger, more dexterous-looking)
        addPart(Mesh::createSphere(0.01f, 6, glm::vec3(1)),
                {0.06f, by - 0.135f, z * 0.85f}, {1.3f, 0.6f, 1.0f}, face * 0.9f, 0.15f);
    }
    // LEGS (shorter than arms)
    for (float z : {0.032f, -0.032f}) {
        addPart(Mesh::createTaperedCylinder(0.016f, 0.013f, 0.05f, 8, glm::vec3(1)),
                {-0.05f, by - 0.055f, z}, {1.0f, 1.0f, 1.0f}, fur * 0.92f, 0.12f);
        addPart(Mesh::createTaperedCylinder(0.013f, 0.010f, 0.045f, 8, glm::vec3(1)),
                {-0.05f, by - 0.10f, z * 0.95f}, {1.0f, 1.0f, 1.0f}, fur * 0.9f, 0.12f);
        // Feet
        addPart(Mesh::createSphere(0.012f, 6, glm::vec3(1)),
                {-0.05f, by - 0.13f, z * 0.9f}, {1.5f, 0.5f, 1.0f}, face * 0.85f, 0.15f);
    }
}

// ─── Common methods ───────────────────────────────────────────────────────

void AnimalEntity::setPosition(const glm::vec3& pos) {
    m_position = pos;
    updateTransforms();
}

void AnimalEntity::setRotation(float yawDegrees) {
    m_yaw = yawDegrees;
    updateTransforms();
}

void AnimalEntity::setHealthColor(float healthPercent) {
    float t = healthPercent / 100.0f;
    for (auto& part : m_parts) {
        glm::vec3 sickColor = part.baseColor * 0.4f + glm::vec3(0.35f, 0.15f, 0.15f);
        part.currentColor = glm::mix(sickColor, part.baseColor, t);
    }
}

void AnimalEntity::animate(float deltaTime, float respRate, float health,
                            float spo2, AnimalStatus status) {
    // Breathing: oscillate chest (part 0 = body) Y-scale
    if (status == AnimalStatus::Alive || status == AnimalStatus::Moribund) {
        float breathSpeed = respRate / 60.0f * 6.28f; // convert breaths/min to rad/s
        m_breathPhase += breathSpeed * deltaTime;
        if (m_breathPhase > 6.28f) m_breathPhase -= 6.28f;
    }

    // Death animation: slowly roll to side
    if (status == AnimalStatus::Dead || status == AnimalStatus::Euthanized) {
        if (!m_isDead) {
            m_isDead = true;
            m_deathTimer = 0.0f;
        }
        m_deathTimer = std::min(1.0f, m_deathTimer + deltaTime * 0.7f);
        // Ease-out: smooth deceleration
        float t = 1.0f - (1.0f - m_deathTimer) * (1.0f - m_deathTimer);
        m_deathRoll = t * 90.0f; // roll 90 degrees onto side
    } else {
        m_isDead = false;
        m_deathTimer = 0.0f;
        m_deathRoll = 0.0f;
    }

    // Cyanosis: tint toward blue when SpO2 drops
    if (spo2 < 95.0f) {
        float cyanosisT = std::min(1.0f, (95.0f - spo2) / 15.0f);
        glm::vec3 blueish(0.5f, 0.5f, 0.75f);
        float healthT = health / 100.0f;
        for (auto& part : m_parts) {
            glm::vec3 sickColor = part.baseColor * 0.4f + glm::vec3(0.35f, 0.15f, 0.15f);
            glm::vec3 healthMixed = glm::mix(sickColor, part.baseColor, healthT);
            part.currentColor = glm::mix(healthMixed, blueish, cyanosisT * 0.5f);
        }
    }

    updateTransforms();
}

void AnimalEntity::getRenderObjects(std::vector<RenderObject>& out) const {
    for (auto& part : m_parts) {
        RenderObject ro;
        ro.mesh = const_cast<Mesh*>(&part.mesh);
        ro.transform = part.worldTransform;
        ro.color = part.currentColor;
        ro.specularStrength = part.specular;
        out.push_back(ro);
    }
}

void AnimalEntity::updateTransforms() {
    glm::mat4 base = glm::translate(glm::mat4(1.0f), m_position);
    base = glm::rotate(base, glm::radians(m_yaw), glm::vec3(0, 1, 0));

    // Death roll: rotate around the forward axis (X) to fall on side
    if (m_deathRoll > 0.01f) {
        base = glm::rotate(base, glm::radians(m_deathRoll), glm::vec3(1, 0, 0));
    }

    base = glm::scale(base, glm::vec3(m_scale));

    // Breathing: apply chest oscillation to the first part (body)
    float breathScale = 1.0f + 0.025f * std::sin(m_breathPhase);

    for (size_t i = 0; i < m_parts.size(); i++) {
        auto& part = m_parts[i];
        glm::mat4 local = glm::translate(glm::mat4(1.0f), part.localOffset);

        // Apply breathing to body parts (first few parts = torso)
        if (i < 3 && !m_isDead) {
            glm::vec3 breathedScale = part.localScale;
            breathedScale.y *= breathScale;
            local = glm::scale(local, breathedScale);
        } else {
            local = glm::scale(local, part.localScale);
        }

        part.worldTransform = base * local;
    }
}

glm::vec3 AnimalEntity::getBodyCenter() const {
    return m_position + glm::vec3(0, 0.02f * m_scale, 0);
}

glm::vec3 AnimalEntity::getHeadPosition() const {
    return m_position + glm::vec3(0.14f * m_scale, 0.02f * m_scale, 0);
}

glm::vec3 AnimalEntity::getTailPosition() const {
    return m_position + glm::vec3(-0.2f * m_scale, 0, 0);
}

glm::vec3 AnimalEntity::getAbdomenPosition() const {
    return m_position + glm::vec3(-0.03f * m_scale, -0.02f * m_scale, 0);
}

glm::vec3 AnimalEntity::getBackPosition() const {
    return m_position + glm::vec3(0, 0.08f * m_scale, 0);
}

} // namespace animsim
