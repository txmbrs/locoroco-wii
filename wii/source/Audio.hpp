#pragma once

namespace Audio {

void Init();
void PlayBGM(int tier = 2);
void StopBGM();
float SingingLevel();
void SetBGMTier(int tier);
void PlayJingle();
void PlayBiteSound();
void PlayJumpSound();
void PlayBounceSound();
void PlaySplitSound();
void PlayMergeSound();
void PlayCallSound(int variation);
void PlayWorldEvent(int sound);
void StopWorldEvent(int sound);

} // namespace Audio
