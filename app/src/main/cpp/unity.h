#pragma once

#include "il2cpp.h"

enum PrimitiveType {
    kPrimitiveSphere = 0,
    kPrimitiveCapsule = 1,
    kPrimitiveCylinder = 2,
    kPrimitiveCube = 3,
    kPrimitivePlane = 4,
    kPrimitiveQuad = 5
};

namespace unity {

bool Initialize();

void *CreatePrimitive(int primitiveType);
void *GetTransform(void *gameObject);
void *GetGameObject(void *component);
void SetActive(void *gameObject, bool active);
void SetName(void *gameObject, const char *name);

Vector3 GetPosition(void *transform);
void SetPosition(void *transform, Vector3 position);
Vector3 GetLocalScale(void *transform);
void SetLocalScale(void *transform, Vector3 scale);
void SetParent(void *transform, void *parent);
void Rotate(void *transform, Vector3 eulerAngles);

void Destroy(void *object);
void LoadScene(int buildIndex);
float DeltaTime();

float Distance(Vector3 a, Vector3 b);

}
