#pragma once

#include <directxtk/SimpleMath.h>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace hlab {

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Quaternion;
using DirectX::SimpleMath::Vector3;
using std::map;
using std::string;
using std::vector;
using std::set;

struct AnimationClip {

    struct Key {
        Vector3 pos = Vector3(0.0f);
        Vector3 scale = Vector3(1.0f);
        Quaternion rot = Quaternion();

        Matrix GetTransform() {
            return Matrix::CreateScale(scale) *
                   Matrix::CreateFromQuaternion(rot) *
                   Matrix::CreateTranslation(pos);
        }
    };
    // ���� ������
    AnimationClip() {}
    AnimationClip(const AnimationClip& other)
        : name(other.name), keys(other.keys), numChannels(other.numChannels),
        numKeys(other.numKeys), duration(other.duration), ticksPerSec(other.ticksPerSec)
    {
        // ���� ���� ����
    }

    // ���� ���� ������
    AnimationClip& operator=(const AnimationClip& other) {
        if (this != &other) {
            name = other.name;               // ���� ����
            keys = other.keys;               // vector�� ���� ���� ����
            numChannels = other.numChannels; // �⺻�� ����
            numKeys = other.numKeys;         // �⺻�� ����
            duration = other.duration;       // �⺻�� ����
            ticksPerSec = other.ticksPerSec; // �⺻�� ����
        }
        return *this;
    }
    // �̵� ���� ������
    AnimationClip& operator=(AnimationClip&& other) noexcept {
        if (this != &other) {  // �ڱ� �ڽſ��� �����ϴ��� Ȯ�� (self-assignment ����)
            // ���� ���ҽ��� ������ �ʿ䰡 �ִٸ� ����
            // name�� keys�� �ڵ����� �����ǹǷ� Ư���� ������ �ʿ� ����

            // ����鿡 ���� �̵� ���� ������ ����
            name = std::move(other.name);         // string�� �̵� ����
            keys = std::move(other.keys);         // vector�� �̵� ����
            numChannels = other.numChannels;      // �⺻���� �̵� �ʿ� ����
            numKeys = other.numKeys;              // �⺻���� �̵� �ʿ� ����
            duration = other.duration;            // �⺻���� �̵� �ʿ� ����
            ticksPerSec = other.ticksPerSec;      // �⺻���� �̵� �ʿ� ����

            // other ��ü�� �ʱ�ȭ ���·� ������ �� ���� (���� ����)
            other.numChannels = 0;
            other.numKeys = 0;
            other.duration = 0.0;
            other.ticksPerSec = 0.0;
        }
        return *this;  // �ڱ� �ڽ� ��ȯ
    }
    string name;              // Name of this animation clip
    vector<vector<Key>> keys; // m_key[boneIdx][frameIdx]
    int numChannels;          // Number of bones
    int numKeys;              // Number of frames of this animation clip
    double duration;          // Duration of animation in ticks
    double ticksPerSec;       // Frames per second
};

struct AnimationData {

    map<string, int32_t> boneNameToId; // �� �̸��� �ε��� ����
    vector<string> boneIdToName; // boneNameToId�� Id ������� �� �̸� ����
    vector<int32_t> boneParents; // �θ� ���� �ε���
    vector<Matrix> offsetMatrices;
    vector<Matrix> boneTransforms;
    vector<AnimationClip> clips;
    //���� �߰�, �ϼ��Ǹ� clips�� ����
    map<string, AnimationClip> clipMaps;
    Matrix defaultTransform;
    Matrix rootTransform = Matrix();
    Matrix accumulatedRootTransform = Matrix();
    //TODO �̰� �� Actor�� Model�� �����°�..��
    Vector3 prevPos = Vector3(0.0f);
    set<int> lowerBodyBones;
    Matrix Get(int clipId, int boneId, int frame) {

        return defaultTransform.Invert() * offsetMatrices[boneId] *
               boneTransforms[boneId] * defaultTransform;

        // defaultTransform�� ���� �о���϶� GeometryGenerator::Normalize()
        // ���� ��� defaultTransform.Invert() * offsetMatrices[boneId]�� �̸�
        // ����ؼ� ��ġ�� defaultTransform * rootTransform�� �̸� ����س���
        // ���� �ֽ��ϴ�. ����� ������ ������ ��ǥ�� ��ȯ ��ʷ� �����Ͻö��
        // ���ܳ����ϴ�.
    }
    Matrix GetAnimationTransform(int clipId, int boneId, int frame) {

        return defaultTransform.Invert() * offsetMatrices[boneId] *
            boneTransforms[boneId] * defaultTransform;

        // defaultTransform�� ���� �о���϶� GeometryGenerator::Normalize()
        // ���� ��� defaultTransform.Invert() * offsetMatrices[boneId]�� �̸�
        // ����ؼ� ��ġ�� defaultTransform * rootTransform�� �̸� ����س���
        // ���� �ֽ��ϴ�. ����� ������ ������ ��ǥ�� ��ȯ ��ʷ� �����Ͻö��
        // ���ܳ����ϴ�.
    }
    Matrix GetAnimationTransform(int InBoneId, Matrix InBoneTransform)
    {
        return defaultTransform.Invert() * offsetMatrices[InBoneId] *
            InBoneTransform * defaultTransform;
    }
    void Update(int clipId, int frame, int type =0) {

        auto &clip = clips[clipId];

        for (int boneId = 0; boneId < boneTransforms.size(); boneId++) {
            // 1�϶� ��ü
            if (type==1) {
              std::set<int>::iterator itr = lowerBodyBones.find(boneId);
              if (itr != lowerBodyBones.end()) {
                continue;
              }
            }
            else if (type==2) {
              std::set<int>::iterator itr = lowerBodyBones.find(boneId);
              if (itr == lowerBodyBones.end()) {
                continue;
              }
            }
            auto &keys = clip.keys[boneId];

            // ����: ��� ä��(��)�� frame ������ �������� ����

            const int parentIdx = boneParents[boneId];
            const Matrix parentMatrix = parentIdx >= 0
                                            ? boneTransforms[parentIdx]
                                            : accumulatedRootTransform;

            // keys.size()�� 0�� ��쿡�� Identity ��ȯ
            auto key = keys.size() > 0
                           ? keys[frame % keys.size()]
                           : AnimationClip::Key(); // key�� reference �ƴ�

            // Root�� ���
            if (parentIdx < 0) {
                if (frame != 0) {
                    accumulatedRootTransform =
                        Matrix::CreateTranslation(key.pos - prevPos) *
                        accumulatedRootTransform;
                } else {
                    auto temp = accumulatedRootTransform.Translation();
                    temp.y = key.pos.y; // ���� ���⸸ ù ���������� ����
                    accumulatedRootTransform.Translation(temp);
                }

                prevPos = key.pos;
                key.pos = Vector3(0.0f); // ��ſ� �̵� ���
            }

            // TODO: parentMatrix ���
            // boneTransforms[boneId] = ...;
            boneTransforms[boneId] = key.GetTransform()*  parentMatrix;
        }
    }

     bool GetBoneTransform(string clipId, int frame, Matrix& InRootTransform, vector<Matrix>&  OutBoneTransform, bool bInit, int type = 0) {
         //TODO ����
        auto& clip = clipMaps[clipId];

        for (int boneId = 0; boneId < boneTransforms.size(); boneId++) {
            // 1�϶� ��ü
            if (type == 1) {
                std::set<int>::iterator itr = lowerBodyBones.find(boneId);
                if (itr != lowerBodyBones.end()) {
                    continue;
                }
            }
            else if (type == 2) {
                std::set<int>::iterator itr = lowerBodyBones.find(boneId);
                if (itr == lowerBodyBones.end()) {
                    continue;
                }
            }
            auto& keys = clip.keys[boneId];

            // ����: ��� ä��(��)�� frame ������ �������� ����

            const int parentIdx = boneParents[boneId];
            const Matrix parentMatrix = parentIdx >= 0
                ? OutBoneTransform[parentIdx]
                : InRootTransform;

            auto key = keys.size() > 0
                ? keys[frame % keys.size()]
                : AnimationClip::Key(); // key�� reference �ƴ�
            // Root�� ���
            if (parentIdx < 0) {
                if (bInit)
                {
                    prevPos = key.pos;
                }
                if (frame != 0) 
                {
                    InRootTransform =
                        Matrix::CreateTranslation(key.pos - prevPos) *
                        InRootTransform;
                }
                else {
                    auto temp = InRootTransform.Translation();
                    temp.y = key.pos.y; // ���� ���⸸ ù ���������� ����
                    InRootTransform.Translation(temp);
                }

                prevPos = key.pos;
                key.pos = Vector3(0.0f); // ��ſ� �̵� ���
            }

            // TODO: parentMatrix ���
            // boneTransforms[boneId] = ...;
            OutBoneTransform[boneId] = key.GetTransform() * parentMatrix;
        }
        return true;
    }
};

} // namespace hlab