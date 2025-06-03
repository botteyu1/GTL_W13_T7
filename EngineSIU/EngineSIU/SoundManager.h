#pragma once
#include <fmod.hpp>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>

#include "fmod_errors.h"
#include "Container/Array.h"
// 사운드 타입을 구분하기 위한 열거형
enum class ESoundType {
    SFX,
    BGM
};

// 사운드 데이터와 타입을 함께 저장하기 위한 구조체
struct SoundData {
    FMOD::Sound* sound = nullptr;
    ESoundType type = ESoundType::SFX;
};

// SFX 채널 정보 (이름과 채널 포인터)
struct SfxChannelInfo {
    std::string name;
    FMOD::Channel* channel;
};

class FSoundManager {
public:
    static FSoundManager& GetInstance() {
        static FSoundManager instance;
        return instance;
    }

    FSoundManager() 
    : system(nullptr), 
      currentBgmChannel(nullptr),
      bgmChannelGroup(nullptr),   // BGM 채널 그룹 포인터 초기화
      currentBGMVolume(0.5f) {} // BGM 기본 볼륨 0.5f (최대)로 초기화

    ~FSoundManager() { Shutdown(); }

    bool Initialize() {
        FMOD_RESULT result = FMOD::System_Create(&system);
        if (result != FMOD_OK) {
            std::cerr << "FMOD System_Create failed! " << FMOD_ErrorString(result) << std::endl;
            return false;
        }

        result = system->init(1024, FMOD_INIT_NORMAL, nullptr);
        if (result != FMOD_OK) {
            std::cerr << "FMOD system init failed! " << FMOD_ErrorString(result) << std::endl;
            if (system) system->release();
            system = nullptr;
            return false;
        }

        // BGM 채널 그룹 생성
        result = system->createChannelGroup("BGMGroup", &bgmChannelGroup);
        if (result != FMOD_OK) {
            std::cerr << "Failed to create BGM channel group: " << FMOD_ErrorString(result) << std::endl;
            bgmChannelGroup = nullptr; // 생성 실패 시 null로 유지
        } else {
            // BGM 채널 그룹의 초기 볼륨 설정
            bgmChannelGroup->setVolume(currentBGMVolume);
        }

        return true;
    }

    void Shutdown() {
        StopAllSounds();

        for (auto& pair : soundMap) {
            if (pair.second.sound) {
                pair.second.sound->release();
            }
        }
        soundMap.clear();

        activeSfxChannelInfos.clear();
        activeNamedSfxChannels.clear();
        currentBgmChannel = nullptr;
        currentBgmName.clear();

        // BGM 채널 그룹 해제
        if (bgmChannelGroup) {
            bgmChannelGroup->release();
            bgmChannelGroup = nullptr;
        }

        if (system) {
            system->close();
            system->release();
            system = nullptr;
        }
    }

    // isLoopingByDefault: SFX의 경우 기본 반복 여부
    bool LoadSound(const std::string& name, const std::string& filePath, ESoundType type, bool isLoopingByDefault = false) {
        if (soundMap.count(name)) {
            return true;
        }

        FMOD_MODE mode = FMOD_CREATECOMPRESSEDSAMPLE;
        if (type == ESoundType::BGM) {
            mode |= FMOD_LOOP_NORMAL;
        } else {
            if (isLoopingByDefault) {
                mode |= FMOD_LOOP_NORMAL;
            } else {
                mode |= FMOD_LOOP_OFF;
            }
        }

        FMOD::Sound* sound = nullptr;
        FMOD_RESULT result = system->createSound(filePath.c_str(), mode, nullptr, &sound);
        if (result != FMOD_OK) {
            std::cerr << "Failed to load sound: " << filePath << " (" << FMOD_ErrorString(result) << ")" << std::endl;
            return false;
        }
        soundMap[name] = {sound, type};
        return true;
    }

    void PlaySound(const std::string& name) {
        auto it = soundMap.find(name);
        if (it == soundMap.end()) {
            std::cerr << "Sound not found to play: " << name << std::endl;
            return;
        }

        const SoundData& data = it->second;
        FMOD::Channel* channel = nullptr;

        if (data.type == ESoundType::BGM) {
            if (currentBgmChannel) {
                 bool isPlaying = false;
                 if (currentBgmChannel->isPlaying(&isPlaying) == FMOD_OK && isPlaying && currentBgmName == name) {
                     return;
                 }
                currentBgmChannel->stop();
                currentBgmChannel = nullptr;
            }
            
            FMOD_RESULT result = system->playSound(data.sound, nullptr, false, &channel); // BGM은 처음에는 false (unpaused) 상태로 재생
            if (result == FMOD_OK && channel) {
                if (bgmChannelGroup) {
                    // 생성된 BGM 채널을 BGM 채널 그룹에 할당
                    // 이렇게 하면 bgmChannelGroup의 볼륨 설정이 이 채널에 자동으로 적용됨
                    channel->setChannelGroup(bgmChannelGroup);
                } else {
                    // BGM 채널 그룹 생성이 실패했다면, 개별 채널에 직접 볼륨 설정
                    channel->setVolume(currentBGMVolume);
                }
                currentBgmChannel = channel;
                currentBgmName = name;
            } else {
                std::cerr << "Failed to play BGM: " << name << " (" << FMOD_ErrorString(result) << ")" << std::endl;
            }
        } else { // SFX
            FMOD_MODE soundMode;
            if (data.sound->getMode(&soundMode) != FMOD_OK) {
                std::cerr << "Failed to get mode for SFX: " << name << std::endl;
                return;
            }
            bool isConfiguredToLoop = (soundMode & FMOD_LOOP_NORMAL);
            if (isConfiguredToLoop) {
                auto activeChannelsMapIt = activeNamedSfxChannels.find(name);
                if (activeChannelsMapIt != activeNamedSfxChannels.end()) {
                    for (FMOD::Channel* existingChannel : activeChannelsMapIt->second) {
                        if (existingChannel) {
                            bool isExistingChannelPlaying = false;
                            FMOD::Sound* soundOnExistingChannel = nullptr;
                            if (existingChannel->isPlaying(&isExistingChannelPlaying) == FMOD_OK && isExistingChannelPlaying &&
                                existingChannel->getCurrentSound(&soundOnExistingChannel) == FMOD_OK &&
                                soundOnExistingChannel == data.sound) {
                                return; 
                            }
                        }
                    }
                }
            }
            FMOD_RESULT result = system->playSound(data.sound, nullptr, false, &channel);
            if (result == FMOD_OK && channel) {
                activeSfxChannelInfos.push_back({name, channel});
                activeNamedSfxChannels[name].push_back(channel);
            } else {
                 std::cerr << "Failed to play SFX: " << name << " (" << FMOD_ErrorString(result) << ")" << std::endl;
            }
        }
    }

    // 특정 사운드(BGM 또는 SFX의 모든 인스턴스) 중지
    void StopSound(const std::string& name) {
        auto it = soundMap.find(name);
        if (it == soundMap.end()) {
            std::cerr << "Sound not found to stop: " << name << std::endl;
            return;
        }

        const SoundData& data = it->second;

        if (data.type == ESoundType::BGM) {
            if (currentBgmChannel && currentBgmName == name) {
                currentBgmChannel->stop();
                currentBgmChannel = nullptr; // Update에서 정리되지만, 즉시 참조 제거
                currentBgmName.clear();
            }
        } else { // SFX
            auto mapIt = activeNamedSfxChannels.find(name);
            if (mapIt != activeNamedSfxChannels.end()) {
                for (FMOD::Channel* ch : mapIt->second) {
                    if (ch) {
                        ch->stop(); // 해당 이름의 모든 SFX 채널 중지
                    }
                }
                // mapIt->second.clear(); // Update에서 정리됨. 여기서 미리 비워도 무방.
            }
        }
    }

    void Update() {
        if (!system) return;
        system->update();

        // BGM 채널 상태 업데이트
        if (currentBgmChannel) {
            bool isPlaying = false;
            FMOD_RESULT result = currentBgmChannel->isPlaying(&isPlaying);
            if (result != FMOD_OK || !isPlaying) {
                currentBgmChannel = nullptr; // 재생 종료 또는 오류
                currentBgmName.clear();
            }
        }

        // SFX 채널들 정리
        activeSfxChannelInfos.erase(
            std::remove_if(activeSfxChannelInfos.begin(), activeSfxChannelInfos.end(),
                [&](SfxChannelInfo& sfxInfo) {
                    if (!sfxInfo.channel) return true; // 유효하지 않은 채널 정보 제거

                    bool isPlaying = false;
                    FMOD_RESULT result = sfxInfo.channel->isPlaying(&isPlaying);
                    if (result != FMOD_OK || !isPlaying) { // 재생 종료 또는 오류
                        // activeNamedSfxChannelsからも該当チャンネルを削除
                        auto mapIt = activeNamedSfxChannels.find(sfxInfo.name);
                        if (mapIt != activeNamedSfxChannels.end()) {
                            mapIt->second.erase(
                                std::remove(mapIt->second.begin(), mapIt->second.end(), sfxInfo.channel),
                                mapIt->second.end()
                            );
                            if (mapIt->second.empty()) { // 해당 이름의 SFX가 더이상 활성 상태가 아니면 맵에서 제거
                                activeNamedSfxChannels.erase(mapIt);
                            }
                        }
                        return true; // activeSfxChannelInfos에서 제거
                    }
                    return false; // 아직 재생 중이면 유지
                }),
            activeSfxChannelInfos.end()
        );
    }

    void StopAllSounds() {
        // BGM 중지
        if (currentBgmChannel) {
            currentBgmChannel->stop();
            currentBgmChannel = nullptr;
            currentBgmName.clear();
        }

        // 모든 SFX 중지
        for (const auto& sfxInfo : activeSfxChannelInfos) {
            if (sfxInfo.channel) {
                sfxInfo.channel->stop();
            }
        }
        activeSfxChannelInfos.clear();
        activeNamedSfxChannels.clear(); // SFX 추적 맵도 클리어

        // (선택 사항) 마스터 채널 그룹 정지 (모든 소리를 즉시 멈춤)
        FMOD::ChannelGroup* masterGroup = nullptr;
        if (system && system->getMasterChannelGroup(&masterGroup) == FMOD_OK && masterGroup) {
            masterGroup->stop();
        }
    }

    TArray<std::string> GetAllSoundNames() const {
        TArray<std::string> names;
        for (const auto& pair : soundMap) {
            names.Add(pair.first);
        }
        return names;
    }
    
    // BGM 볼륨 설정 (0.0f ~ 1.0f)
    void SetBGMVolume(float volume) {
        currentBGMVolume = std::max(0.0f, std::min(1.0f, volume)); // 값 범위 제한
        if (bgmChannelGroup) {
            bgmChannelGroup->setVolume(currentBGMVolume);
        } else if (currentBgmChannel) {
            // BGM 채널 그룹이 없는 경우 (예: Initialize 실패), 현재 재생 중인 BGM 채널에 직접 설정
            currentBgmChannel->setVolume(currentBGMVolume);
        }
    }

    // 현재 설정된 BGM 볼륨 가져오기
    float GetBGMVolume() const {
        // FMOD에서 직접 가져올 수도 있지만, 우리가 설정한 값을 반환하는 것이 더 일관적일 수 있음
        // if (bgmChannelGroup) {
        //     float volume;
        //     bgmChannelGroup->getVolume(&volume);
        //     return volume;
        // }
        return currentBGMVolume;
    }
    
private:
    FSoundManager(const FSoundManager&) = delete;
    FSoundManager& operator=(const FSoundManager&) = delete;

    FMOD::System* system;
    std::unordered_map<std::string, SoundData> soundMap;

    // BGM 관리
    FMOD::Channel* currentBgmChannel;
    std::string currentBgmName;
    FMOD::ChannelGroup* bgmChannelGroup; // BGM 전용 채널 그룹
    float currentBGMVolume;              // 현재 BGM 볼륨 설정 (0.0 ~ 1.0)

    // SFX 관리
    std::vector<SfxChannelInfo> activeSfxChannelInfos; // Update에서 정리하기 위한 주 SFX 채널 목록
    std::unordered_map<std::string, std::vector<FMOD::Channel*>> activeNamedSfxChannels; // 이름으로 SFX를 중지하기 위한 맵

};
