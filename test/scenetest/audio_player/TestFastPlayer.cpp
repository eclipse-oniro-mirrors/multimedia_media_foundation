/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gtest/gtest.h"
#include <chrono>
#include <fcntl.h>
#include <cmath>
#include <thread>
#include "foundation/log.h"
#include "scenetest/helper/test_player.hpp"

#ifndef WIN32
#define RESOURCE_DIR "/data/test/media/"
using namespace testing::ext;
#endif

using namespace OHOS::Media::Plugin;
using namespace OHOS::Media::Test;

namespace OHOS {
namespace Media {
namespace Test {
    class UtTestFastPlayer : public ::testing::Test {
    public:
        void SetUp() override
        {
        }
        void TearDown() override
        {
        }
    };

    bool CheckTimeEquality(int32_t expectValue, int32_t currentValue)
    {
        MEDIA_LOG_I("expectValue : %d, currentValue : %d", expectValue, currentValue);
        return fabs(expectValue - currentValue) < 1000; // if open debug log, should use value >= 1000
    }

    void TestSinglePlayerFinishedAutomatically(std::string url)
    {
        std::unique_ptr<TestPlayer> player = TestPlayer::Create();
        ASSERT_EQ(0, player->SetSource(TestSource(url)));
        ASSERT_EQ(0, player->Prepare());
        ASSERT_EQ(0, player->Play());
        while (player->IsPlaying()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1000 MS
        }
    }

    void TestSinglePlayerFinishedManually(std::string url)
    {
        std::unique_ptr<TestPlayer> player = TestPlayer::Create();
        ASSERT_EQ(0, player->SetSource(TestSource(url)));
        ASSERT_EQ(0, player->Prepare());
        ASSERT_EQ(0, player->Play());
        std::vector<OHOS::Media::Format> audioTrack;
        ASSERT_EQ(0, player->GetAudioTrackInfo(audioTrack));
        ASSERT_TRUE(player->IsPlaying());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1000 MS
        ASSERT_EQ(0, player->Pause());
        ASSERT_EQ(0, player->Stop());
        ASSERT_EQ(0, player->Reset());
        ASSERT_EQ(0, player->Release());
    }

    void TestSinglePlayerGetDuration(std::string url, int32_t expectDuration)
    {
        std::unique_ptr<TestPlayer> player = TestPlayer::Create();
        ASSERT_EQ(0, player->SetSource(TestSource(url)));
        ASSERT_EQ(0, player->Prepare());
        ASSERT_EQ(0, player->Play());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1000 MS
        int64_t duration;
        ASSERT_EQ(0, player->GetDuration(duration));
        ASSERT_TRUE(CheckTimeEquality(expectDuration, duration));
        ASSERT_EQ(0, player->Stop());
    }

    void TestSinglePlayerGetCurrentTime(std::string url)
    {
        int64_t currentMS {0};
        std::unique_ptr<TestPlayer> player = TestPlayer::Create();
        ASSERT_EQ(0, player->SetSource(TestSource(url)));
        ASSERT_EQ(0, player->Prepare());
        ASSERT_EQ(0, player->Play());
        ASSERT_TRUE(player->IsPlaying());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1000 MS
        ASSERT_EQ(0, player->GetCurrentTime(currentMS));
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1000 MS
        ASSERT_TRUE(CheckTimeEquality(1000, currentMS)); // 1000 MS
        ASSERT_EQ(0, player->Stop());
    }

    void TestSinglePlayerSeek(std::string url)
    {
        int64_t seekPos {5000};
        int64_t currentMS {0};
        std::unique_ptr<TestPlayer> player = TestPlayer::Create();
        ASSERT_EQ(0, player->SetSource(TestSource(url)));
        ASSERT_EQ(0, player->Prepare());
        ASSERT_EQ(0, player->Play());
        ASSERT_TRUE(player->IsPlaying());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1000 MS
        ASSERT_EQ(0, player->Seek(seekPos));
        ASSERT_EQ(0, player->GetCurrentTime(currentMS));
        EXPECT_TRUE(CheckTimeEquality(seekPos, currentMS));
        std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 2000 MS
        ASSERT_EQ(0, player->Stop());
    }

    void TestSinglePlayerSeekPause(std::string url)
    {
        int64_t seekPos {5000}; // 5000 MS
        int64_t currentMS {0};
        std::unique_ptr<TestPlayer> player = TestPlayer::Create();
        ASSERT_EQ(0, player->SetSource(TestSource(url)));
        ASSERT_EQ(0, player->Prepare());
        ASSERT_EQ(0, player->Play());
        ASSERT_TRUE(player->IsPlaying());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1000 MS
        ASSERT_EQ(0, player->Pause());
        ASSERT_EQ(0, player->Seek(seekPos));
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1000 MS
        ASSERT_EQ(0, player->GetCurrentTime(currentMS));
        ASSERT_TRUE(CheckTimeEquality(seekPos, currentMS)); // pause + seek still pause
        ASSERT_EQ(0, player->Stop());
    }

    void TestSinglePlayerPauseSeek(std::string url)
    {
        int64_t seekPos {5000}; // 5000 MS
        int64_t currentMS {0};
        std::unique_ptr<TestPlayer> player = TestPlayer::Create();
        ASSERT_EQ(0, player->SetSource(TestSource(url)));
        ASSERT_EQ(0, player->Prepare());
        ASSERT_EQ(0, player->Play());
        ASSERT_TRUE(player->IsPlaying());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1000 MS
        ASSERT_EQ(0, player->Pause());
        ASSERT_EQ(0, player->Seek(seekPos));
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1000 MS
        ASSERT_EQ(0, player->GetCurrentTime(currentMS));
        ASSERT_TRUE(CheckTimeEquality(seekPos, currentMS)); // pause + seek still pause
        ASSERT_EQ(0, player->Stop());
    }

    void TestSinglePlayerSingleLoop(std::string url)
    {
        std::unique_ptr<TestPlayer> player = TestPlayer::Create();
        ASSERT_EQ(0, player->SetSource(TestSource(url)));
        ASSERT_EQ(0, player->SetSingleLoop(true));
        ASSERT_EQ(0, player->Prepare());
        ASSERT_EQ(0, player->Play());
        std::this_thread::sleep_for(std::chrono::seconds(20)); // 20 second
        ASSERT_EQ(0, player->Stop());
    }

    void TestSinglePlayerHttp(std::string url)
    {
        std::unique_ptr<TestPlayer> player = TestPlayer::Create();
        ASSERT_EQ(0, player->SetSource(TestSource(url)));
        ASSERT_EQ(0, player->Prepare());
        ASSERT_EQ(0, player->Play());
        std::this_thread::sleep_for(std::chrono::seconds(20)); // 20 second
        ASSERT_EQ(0, player->Seek(10000)); // 10000 ms
        std::this_thread::sleep_for(std::chrono::seconds(10)); // 10 second
        ASSERT_EQ(0, player->Stop());
    }

    void TestSinglePlayerHls(std::string url)
    {
        std::unique_ptr<TestPlayer> player = TestPlayer::Create();
        ASSERT_EQ(0, player->SetSource(TestSource(url)));
        ASSERT_EQ(0, player->Prepare());
        ASSERT_EQ(0, player->Play());
        std::this_thread::sleep_for(std::chrono::seconds(5)); // 5 second
        ASSERT_EQ(0, player->Stop());
    }

    void TestSinglePlayerSeekNearEnd(std::string url, int32_t expectDuration)
    {
        std::unique_ptr<TestPlayer> player = TestPlayer::Create();
        ASSERT_EQ(0, player->SetSource(TestSource(url)));
        ASSERT_EQ(0, player->Prepare());
        ASSERT_EQ(0, player->Play());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1000 MS
        int64_t duration;
        ASSERT_EQ(0, player->GetDuration(duration));
        ASSERT_TRUE(CheckTimeEquality(expectDuration, duration));
        int32_t count = 5;
        while (count > 0) {
            NZERO_LOG(player->Seek(duration - 100)); // 100 MS
            NZERO_LOG(player->Seek(0));
            count--;
        }
        ASSERT_EQ(0, player->Reset());
        ASSERT_EQ(0, player->Release());
    }

    void TestSinglePlayerFinishPlayAgain(std::string url)
    {
        std::unique_ptr<TestPlayer> player = TestPlayer::Create();
        ASSERT_EQ(0, player->SetSource(TestSource(url)));
        ASSERT_EQ(0, player->Prepare());
        ASSERT_EQ(0, player->Play());
        while (player->IsPlaying()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1000 MS
        }
        ASSERT_EQ(0, player->Play());
        while (player->IsPlaying()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1000 MS
        }
    }

    HST_TEST(UtTestFastPlayer, TestSinglePlayerFinishedAutomatically, TestSize.Level1)
    {
        std::vector<std::string> vecSource;
        vecSource.push_back(std::string(RESOURCE_DIR "/MP3/MP3_LONG_48000_32.mp3"));
        vecSource.push_back(std::string(RESOURCE_DIR "/MP3/MP3_48000_32_SHORT.mp3"));
        vecSource.push_back(std::string(RESOURCE_DIR "/AAC/AAC_48000_32_SHORT.aac"));
        vecSource.push_back(std::string(RESOURCE_DIR "/FLAC/vorbis_48000_32_SHORT.flac"));
        vecSource.push_back(std::string(RESOURCE_DIR "/M4A/MPEG-4_48000_32_SHORT.m4a"));
        vecSource.push_back(std::string(RESOURCE_DIR "/WAV/vorbis_48000_32_SHORT.wav"));
        for (auto url : vecSource)
        {
            TestSinglePlayerFinishedAutomatically(url);
        }
    }

    HST_TEST(UtTestFastPlayer, TestSinglePlayerFinishedManually, TestSize.Level1)
    {
        std::vector<std::string> vecSource;
        vecSource.push_back(std::string(RESOURCE_DIR "/M4A/MPEG-4_48000_32_SHORT.m4a"));
        vecSource.push_back(std::string(RESOURCE_DIR "/WAV/vorbis_48000_32_SHORT.wav"));
        vecSource.push_back(std::string(RESOURCE_DIR "/AAC/AAC_48000_32_SHORT.aac"));
        vecSource.push_back(std::string(RESOURCE_DIR "/FLAC/vorbis_48000_32_SHORT.flac"));
        vecSource.push_back(std::string(RESOURCE_DIR "/MP3/MP3_LONG_48000_32.mp3"));
        vecSource.push_back(std::string(RESOURCE_DIR "/WAV/vorbis_48000_32_SHORT.wav"));
        for (auto url : vecSource)
        {
            TestSinglePlayerFinishedManually(url);
        }
    }

    HST_TEST(UtTestFastPlayer, TestSinglePlayerGetDuration, TestSize.Level1)
    {
        std::vector<std::string> vecSource;
        vecSource.push_back(std::string(RESOURCE_DIR "/MP3/MP3_LONG_48000_32.mp3"));
        vecSource.push_back(std::string(RESOURCE_DIR "/M4A/MPEG-4_48000_32_LONG.m4a"));
        for (auto url : vecSource)
        {
            TestSinglePlayerGetDuration(url, 30000); // 30000 MS
        }
        TestSinglePlayerGetDuration(std::string(RESOURCE_DIR "/WAV/vorbis_48000_32_SHORT.wav"),
                                        5000); // 5000 MS
    }

    HST_TEST(UtTestFastPlayer, TestSinglePlayerGetCurrentTime, TestSize.Level1)
    {
        std::vector<std::string> vecSource;
        vecSource.push_back(std::string(RESOURCE_DIR "/MP3/MP3_LONG_48000_32.mp3"));
        vecSource.push_back(std::string(RESOURCE_DIR "/M4A/MPEG-4_48000_32_LONG.m4a"));
        vecSource.push_back(std::string("http://img.51miz.com/preview/sound/00/26/73/51miz-S267356-423D33372.mp3"));
        for (auto url : vecSource)
        {
            TestSinglePlayerGetCurrentTime(url);
        }
    }

    HST_TEST(UtTestFastPlayer, TestSinglePlayerSeek, TestSize.Level1)
    {
        std::vector<std::string> vecSource;
        vecSource.push_back(std::string(RESOURCE_DIR "/MP3/MP3_LONG_48000_32.mp3"));
        vecSource.push_back(std::string(RESOURCE_DIR "/M4A/MPEG-4_48000_32_LONG.m4a"));
        vecSource.push_back(std::string("http://img.51miz.com/preview/sound/00/26/73/51miz-S267356-423D33372.mp3"));
        for (auto url : vecSource)
        {
            TestSinglePlayerSeek(url);
        }
    }

    HST_TEST(UtTestFastPlayer, TestSinglePlayerSeekPause, TestSize.Level1)
    {
        std::vector<std::string> vecSource;
        vecSource.push_back(std::string(RESOURCE_DIR "/MP3/MP3_LONG_48000_32.mp3"));
        vecSource.push_back(std::string(RESOURCE_DIR "/M4A/MPEG-4_48000_32_LONG.m4a"));
        vecSource.push_back(std::string("http://img.51miz.com/preview/sound/00/26/73/51miz-S267356-423D33372.mp3"));
        for (auto url : vecSource)
        {
            TestSinglePlayerSeekPause(url);
        }
    }

    HST_TEST(UtTestFastPlayer, TestSinglePlayerPauseSeek, TestSize.Level1)
    {
        std::vector<std::string> vecSource;
        vecSource.push_back(std::string(RESOURCE_DIR "/MP3/MP3_LONG_48000_32.mp3"));
        vecSource.push_back(std::string(RESOURCE_DIR "/M4A/MPEG-4_48000_32_LONG.m4a"));
        vecSource.push_back(std::string("http://img.51miz.com/preview/sound/00/26/73/51miz-S267356-423D33372.mp3"));
        for (auto url : vecSource)
        {
            TestSinglePlayerPauseSeek(url);
        }
    }

    HST_TEST(UtTestFastPlayer, TestSinglePlayerSingleLoop, TestSize.Level1)
    {
        std::vector<std::string> vecSource;
        vecSource.push_back(std::string(RESOURCE_DIR "/M4A/MPEG-4_48000_32_SHORT.m4a"));
        vecSource.push_back(std::string(RESOURCE_DIR "/WAV/vorbis_48000_32_SHORT.wav"));
        vecSource.push_back(std::string(RESOURCE_DIR "/AAC/AAC_48000_32_SHORT.aac"));
        vecSource.push_back(std::string(RESOURCE_DIR "/FLAC/vorbis_48000_32_SHORT.flac"));
        vecSource.push_back(std::string(RESOURCE_DIR "/MP3/MP3_LONG_48000_32.mp3"));
        vecSource.push_back(std::string(RESOURCE_DIR "/WAV/vorbis_48000_32_SHORT.wav"));
        for (auto url : vecSource)
        {
            TestSinglePlayerSingleLoop(url);
        }
    }

    HST_TEST(UtTestFastPlayer, TestSinglePlayerHttp, TestSize.Level1)
    {
        std::vector<std::string> vecSource;
        vecSource.push_back(std::string("http://img.51miz.com/preview/sound/00/26/73/51miz-S267356-423D33372.mp3"));
        vecSource.push_back(std::string("https://img.51miz.com/preview/sound/00/26/73/51miz-S267356-423D33372.mp3"));
        for (auto url : vecSource)
        {
            TestSinglePlayerHttp(url);
        }
    }

    HST_TEST(UtTestFastPlayer, TestSinglePlayerHls, TestSize.Level1)
    {
        std::vector<std::string> vecSource;
        vecSource.push_back(std::string("http://220.161.87.62:8800/hls/2/index.m3u8"));
        for (auto url : vecSource)
        {
            TestSinglePlayerHls(url);
        }
    }

    HST_TEST(UtTestFastPlayer, TestSinglePlayerSeekNearEnd, TestSize.Level1)
    {
        std::vector<std::string> vecSource;
        vecSource.push_back(std::string(RESOURCE_DIR "/MP3/MP3_LONG_48000_32.mp3"));
        vecSource.push_back(std::string(RESOURCE_DIR "/M4A/MPEG-4_48000_32_LONG.m4a"));
        for (auto url : vecSource)
        {
            TestSinglePlayerSeekNearEnd(url, 30000); // 30000 MS
        }
    }

    HST_TEST(UtTestFastPlayer, TestSinglePlayerFinishPlayAgain, TestSize.Level1)
    {
        std::vector<std::string> vecSource;
        vecSource.push_back(std::string(RESOURCE_DIR "/M4A/MPEG-4_48000_32_SHORT.m4a"));
        vecSource.push_back(std::string(RESOURCE_DIR "/WAV/vorbis_48000_32_SHORT.wav"));
        vecSource.push_back(std::string(RESOURCE_DIR "/AAC/AAC_48000_32_SHORT.aac"));
        vecSource.push_back(std::string(RESOURCE_DIR "/FLAC/vorbis_48000_32_SHORT.flac"));
        vecSource.push_back(std::string(RESOURCE_DIR "/MP3/MP3_LONG_48000_32.mp3"));
        vecSource.push_back(std::string(RESOURCE_DIR "/WAV/vorbis_48000_32_SHORT.wav"));
        for (auto url : vecSource)
        {
            TestSinglePlayerFinishPlayAgain(url);
        }
    }

    } // namespace Test
} // namespace Media
} // namespace OHOS
