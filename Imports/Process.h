#pragma once

// Standard library includes first
#include <cmath>
#include <cstdio>
#include <string>
#include <cstdint>

// Unity includes
#include "../Unity/Vector3.h"

// Configuration includes (must come before Utils.h and Offsets.h which use them)
#include "../Cfg/strenc.h"
#include "../Cfg/encrypt.hh"

// UTF8 utilities
#include "UTF8.h"

// Core utilities
#include "Utils.h"
#include "Offsets.h"

// Scope (includes Auth)
#include "Scope.h"

// Declarações externas
extern int SWidth;
extern int SHeight;

struct Vector4_2 {
    float X;
    float Y;
    float Z;
    float W;
};

struct Matrix {
    Vector4_2 Position;
    Vector4_2 Rotation;
    Vector4_2 Scale;
};

struct UnityMatrix {
    float _11, _12, _13, _14;
    float _21, _22, _23, _24;
    float _31, _32, _33, _34;
    float _41, _42, _43, _44;
};

inline std::string ObterStr(uintptr_t address, int count) {
    int classname;
    int m = 0;
    char a[500];
    UTF8 buf88[256] = "";
    UTF16 buf16[34] = { 0 };
    int hex[2] = { 0 };
    for (int i = 0; i < count; i++)
    {
        classname = Ler<uintptr_t>(address + i * 4);
        hex[0] = (classname & 0xfffff000) >> 16;
        hex[1] = classname & 0xffff;
        buf16[m] = hex[1];
        buf16[m + 1] = hex[0];
        m += 2;
    }
    Utf16_To_Utf8(buf16, buf88, sizeof(buf88), strictConversion);
    sprintf_s(a, "%s", buf88);
    return a;
}

inline Vector3 Transform$$ObterPosicao(uintptr_t Transform) { // ChromeX - sufxzada
    uintptr_t TransformAcess = Ler<uintptr_t>(Transform + string2Offset(AY_OBFUSCATE("0x8")));
    int TransformIndex = Ler<int>(TransformAcess + string2Offset(AY_OBFUSCATE("0x24")));
    uintptr_t pTransformValues = Ler<uintptr_t>(Ler<uintptr_t>(TransformAcess + string2Offset(AY_OBFUSCATE("0x20"))) + string2Offset(AY_OBFUSCATE("0x10")));
    int offsetPosition = string2Offset(AY_OBFUSCATE("0x30")) * TransformIndex;
    Vector3 ResultPosition = Ler<Vector3>(pTransformValues + offsetPosition);
    uintptr_t pOffsetCount = Ler<uintptr_t>(Ler<uintptr_t>(TransformAcess + string2Offset(AY_OBFUSCATE("0x20"))) + string2Offset(AY_OBFUSCATE("0x14")));
    int IndexTransform = Ler<int>(pOffsetCount + string2Offset(AY_OBFUSCATE("0x4")) * TransformIndex);
    int curIndex = 0;
    while (IndexTransform >= 0) {
        curIndex++;
        if (curIndex >= 60) {
            return ResultPosition;
        }
        Matrix matriz = Ler<Matrix>(pTransformValues + sizeof(Matrix) * IndexTransform);

        float RotationX = matriz.Rotation.X;
        float RotationY = matriz.Rotation.Y;
        float RotationZ = matriz.Rotation.Z;
        float RotationW = matriz.Rotation.W;

        float ScaleX = ResultPosition.X * matriz.Scale.X;
        float ScaleY = ResultPosition.Y * matriz.Scale.Y;
        float ScaleZ = ResultPosition.Z * matriz.Scale.Z;

        ResultPosition.X = matriz.Position.X + ScaleX + (ScaleX * ((RotationY * RotationY * -2.0) - (RotationZ * RotationZ * 2.0))) + (ScaleY * ((RotationW * RotationZ * -2.0) - (RotationY * RotationX * -2.0))) + (ScaleZ * ((RotationZ * RotationX * 2.0) - (RotationW * RotationY * -2.0)));
        ResultPosition.Y = matriz.Position.Y + ScaleY + (ScaleX * ((RotationX * RotationY * 2.0) - (RotationW * RotationZ * -2.0))) + (ScaleY * ((RotationZ * RotationZ * -2.0) - (RotationX * RotationX * 2.0))) + (ScaleZ * ((RotationW * RotationX * -2.0) - (RotationZ * RotationY * -2.0)));
        ResultPosition.Z = matriz.Position.Z + ScaleZ + (ScaleX * ((RotationW * RotationY * -2.0) - (RotationX * RotationZ * -2.0))) + (ScaleY * ((RotationY * RotationZ * 2.0) - (RotationW * RotationX * -2.0))) + (ScaleZ * ((RotationX * RotationX * -2.0) - (RotationY * RotationY * 2.0)));

        IndexTransform = Ler<int>(pOffsetCount + string2Offset(AY_OBFUSCATE("0x4")) * IndexTransform);
    }
    return ResultPosition;
}

inline void Transform$$DefinirPosicao(uintptr_t Transform, Vector3 NovaPosicao) {
    uintptr_t TransformAcess = Ler<uintptr_t>(Transform + string2Offset(AY_OBFUSCATE("0x8")));
    int TransformIndex = Ler<int>(TransformAcess + string2Offset(AY_OBFUSCATE("0x24")));
    uintptr_t pTransformValues = Ler<uintptr_t>(Ler<uintptr_t>(TransformAcess + string2Offset(AY_OBFUSCATE("0x20"))) + string2Offset(AY_OBFUSCATE("0x10")));
    int offsetPosition = string2Offset(AY_OBFUSCATE("0x30")) * TransformIndex;
    Escrever<Vector3>(pTransformValues + offsetPosition, NovaPosicao);
}

static auto GetPlayerPosition = [](uintptr_t player, int positionType) -> Vector3 { // ChromeX - sufxzada
    if (positionType == 0) {
        uintptr_t m_CachedTransform = Ler<uintptr_t>(player + string2Offset(AY_OBFUSCATE("0x34")));
        if (m_CachedTransform == 0) return Vector3{ 0, 0, 0 };

        return Transform$$ObterPosicao(m_CachedTransform);
    }

    if (positionType == 1) {
        uintptr_t OLCJOGDHJJJ = Ler<uintptr_t>(player + string2Offset(AY_OBFUSCATE("0x3F8")));
        if (OLCJOGDHJJJ == 0) return Vector3{ 0, 0, 0 };

        uintptr_t TFNode = Ler<uintptr_t>(OLCJOGDHJJJ + string2Offset(AY_OBFUSCATE("0x8")));
        if (TFNode == 0) return Vector3{ 0, 0, 0 };

        return Transform$$ObterPosicao(TFNode);
    }
    return Vector3{ 0, 0, 0 };
    };

inline Vector3 ObterOssos(uintptr_t Player, uintptr_t Position) { // ChromeX
    uintptr_t ListTransform = Ler<uintptr_t>(Player + string2Offset(AY_OBFUSCATE("0x6F0")));
    if (ListTransform != string2Offset(AY_OBFUSCATE("0")))
    {
        uintptr_t Transform = Ler<uintptr_t>(ListTransform + string2Offset(AY_OBFUSCATE("0x8")));
        if (Transform != string2Offset(AY_OBFUSCATE("0")))
        {
            uintptr_t Location = Ler<uintptr_t>(Transform + Position);
            if (Location != string2Offset(AY_OBFUSCATE("0")))
            {
                uintptr_t H1 = Ler<uintptr_t>(Location + string2Offset(AY_OBFUSCATE("0x8")));
                if (H1 != string2Offset(AY_OBFUSCATE("0")))
                {
                    uintptr_t H2 = Ler<uintptr_t>(H1 + string2Offset(AY_OBFUSCATE("0x28")));
                    if (H2 != string2Offset(AY_OBFUSCATE("0")))
                    {
                        uintptr_t H3 = Ler<uintptr_t>(H2 + string2Offset(AY_OBFUSCATE("0x14")));
                        if (H3 != string2Offset(AY_OBFUSCATE("0")))
                        {
                            uintptr_t H4 = H3 + string2Offset(AY_OBFUSCATE("0x60"));
                            if (H4 != string2Offset(AY_OBFUSCATE("0")))
                            {
                                return Ler<Vector3>(H4);
                            }
                        }
                    }
                }
            }
        }
    }
    return Vector3(0,0,0);
}

inline Vector3 GetChestPosition(uintptr_t Entidade) {
    static uintptr_t posChest = 0;
    bool Garota = Ler<bool>(Entidade + Offsets.isfemale);

    // offsets do peito
    if (Garota) {
        posChest = string2Offset(AY_OBFUSCATE("0x10"));
    }
    else {
        posChest = string2Offset(AY_OBFUSCATE("0x10"));
    }

    uintptr_t ListTransform = Ler<uintptr_t>(Entidade + string2Offset(AY_OBFUSCATE("0x6F0")));
    if (ListTransform != string2Offset(AY_OBFUSCATE("0"))) {
        uintptr_t Transform = Ler<uintptr_t>(ListTransform + string2Offset(AY_OBFUSCATE("0x8")));
        if (Transform != string2Offset(AY_OBFUSCATE("0"))) {
            uintptr_t Location = Ler<uintptr_t>(Transform + posChest);
            if (Location != string2Offset(AY_OBFUSCATE("0"))) {
                uintptr_t H1 = Ler<uintptr_t>(Location + string2Offset(AY_OBFUSCATE("0x8")));
                if (H1 != string2Offset(AY_OBFUSCATE("0"))) {
                    uintptr_t H2 = Ler<uintptr_t>(H1 + string2Offset(AY_OBFUSCATE("0x28")));
                    if (H2 != string2Offset(AY_OBFUSCATE("0"))) {
                        uintptr_t H3 = Ler<uintptr_t>(H2 + string2Offset(AY_OBFUSCATE("0x14")));
                        if (H3 != string2Offset(AY_OBFUSCATE("0"))) {
                            uintptr_t H4 = H3 + string2Offset(AY_OBFUSCATE("0x60"));
                            if (H4 != string2Offset(AY_OBFUSCATE("0"))) {
                                return Ler<Vector3>(H4);
                            }
                        }
                    }
                }
            }
        }
    }
    return Vector3(0, 0, 0);
}

inline Vector3 GetHeadPosition(uintptr_t Entidade) { // ChromeX
    static uintptr_t posHead = 0;
    bool Garota = Ler<bool>(Entidade + Offsets.isfemale);
    if (Garota) {
        posHead = string2Offset(AY_OBFUSCATE("0x3C"));
    }
    else {
        posHead = string2Offset(AY_OBFUSCATE("0x38"));
    }
    uintptr_t ListTransform = Ler<uintptr_t>(Entidade + string2Offset(AY_OBFUSCATE("0x6F0")));
    if (ListTransform != string2Offset(AY_OBFUSCATE("0"))) {
        uintptr_t Transform = Ler<uintptr_t>(ListTransform + string2Offset(AY_OBFUSCATE("0x8")));
        if (Transform != string2Offset(AY_OBFUSCATE("0"))) {
            uintptr_t Location = Ler<uintptr_t>(Transform + posHead);
            if (Location != string2Offset(AY_OBFUSCATE("0"))) {
                uintptr_t H1 = Ler<uintptr_t>(Location + string2Offset(AY_OBFUSCATE("0x8")));
                if (H1 != string2Offset(AY_OBFUSCATE("0"))) {
                    uintptr_t H2 = Ler<uintptr_t>(H1 + string2Offset(AY_OBFUSCATE("0x28")));
                    if (H2 != string2Offset(AY_OBFUSCATE("0"))) {
                        uintptr_t H3 = Ler<uintptr_t>(H2 + string2Offset(AY_OBFUSCATE("0x14")));
                        if (H3 != string2Offset(AY_OBFUSCATE("0"))) {
                            uintptr_t H4 = H3 + string2Offset(AY_OBFUSCATE("0x60"));
                            if (H4 != string2Offset(AY_OBFUSCATE("0"))) {
                                return Ler<Vector3>(H4);
                            }
                        }
                        else {
                            return Transform$$ObterPosicao(Ler<uintptr_t>(Ler<uintptr_t>(Entidade + Offsets.HeadTransform) + string2Offset(AY_OBFUSCATE("0x8"))));
                        }
                    }
                    else {
                        return Transform$$ObterPosicao(Ler<uintptr_t>(Ler<uintptr_t>(Entidade + Offsets.HeadTransform) + string2Offset(AY_OBFUSCATE("0x8"))));
                    }
                }
                else {
                    return Transform$$ObterPosicao(Ler<uintptr_t>(Ler<uintptr_t>(Entidade + Offsets.HeadTransform) + string2Offset(AY_OBFUSCATE("0x8"))));
                }
            }
            else {
                return Transform$$ObterPosicao(Ler<uintptr_t>(Ler<uintptr_t>(Entidade + Offsets.HeadTransform) + string2Offset(AY_OBFUSCATE("0x8"))));
            }
        }
        else {
            return Transform$$ObterPosicao(Ler<uintptr_t>(Ler<uintptr_t>(Entidade + Offsets.HeadTransform) + string2Offset(AY_OBFUSCATE("0x8"))));
        }
    }
    else {
        return Transform$$ObterPosicao(Ler<uintptr_t>(Ler<uintptr_t>(Entidade + Offsets.HeadTransform) + string2Offset(AY_OBFUSCATE("0x8"))));
    }
    return Vector3(0, 0, 0);
}

inline bool IsBoneValid(const Vector3& bone) {
    return (bone.Z > 0.0f && bone.X >= 0 && bone.X <= SWidth && bone.Y >= 0 && bone.Y <= SHeight && !std::isnan(bone.X) && !std::isnan(bone.Y) && !std::isnan(bone.Z));
}

inline struct Vector3 World2Screen(struct UnityMatrix viewMatrix, struct Vector3 pos) {
    struct Vector3 screen;
    float clipW = (viewMatrix._14 * pos.X) + (viewMatrix._24 * pos.Y) + (viewMatrix._34 * pos.Z) + viewMatrix._44;
    if (clipW < 0.01f) {
        screen.Z = -1.0f;
        return screen;
    }
    float clipX = (viewMatrix._11 * pos.X) + (viewMatrix._21 * pos.Y) + (viewMatrix._31 * pos.Z) + viewMatrix._41;
    float clipY = (viewMatrix._12 * pos.X) + (viewMatrix._22 * pos.Y) + (viewMatrix._32 * pos.Z) + viewMatrix._42;
    screen.X = (SWidth / 2.0f) + (SWidth / 2.0f) * (clipX / clipW);
    screen.Y = (SHeight / 2.0f) - (SHeight / 2.0f) * (clipY / clipW);
    screen.Z = clipW;
    return screen;
}

inline bool isInsideFov(int x, int y) {
    int circle_x = SWidth / string2Offset(AY_OBFUSCATE("2"));
    int circle_y = SHeight / string2Offset(AY_OBFUSCATE("2"));
    int rad = 180 * string2Offset(AY_OBFUSCATE("8"));
    return (x - circle_x) * (x - circle_x) + (y - circle_y) * (y - circle_y) <= rad * rad;
}

inline void NetworkInit() {
    if (!ConnectEmulator()) {
        return;
    }
    int tentativas = 0;
    while (il2cpp == 0) {
        il2cpp = ObterEnderecoDaBiblioteca(AY_OBFUSCATE("libil2cpp.so"));
        if (il2cpp == 0) {
            Sleep(1500);
            ++tentativas;
        }
    }
    isAttached = true;
}