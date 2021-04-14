// Copyright (c) 2013-2020 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: https://opensource.org/licenses/AFL-3.0

#pragma once

#include "SSVOpenHexagon/Data/LevelData.hpp"
#include "SSVOpenHexagon/Data/PackData.hpp"
#include "SSVOpenHexagon/Data/ProfileData.hpp"
#include "SSVOpenHexagon/Data/StyleData.hpp"
#include "SSVOpenHexagon/Data/LoadInfo.hpp"
#include "SSVOpenHexagon/Data/PackInfo.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace sf {
class SoundBuffer;
class Font;
class Texture;
} // namespace sf

namespace ssvu::FileSystem {
class Path;
}

namespace ssvufs = ssvu::FileSystem;

namespace hg {

namespace Steam {
class steam_manager;
}

class MusicData;
class AssetStorage;

class HGAssets
{
private:
    Steam::steam_manager* steamManager;

    bool levelsOnly{false};

    std::unique_ptr<AssetStorage> assetStorage;

    std::unordered_map<std::string, LevelData> levelDatas;
    std::unordered_map<std::string, std::vector<std::string>>
        levelDataIdsByPack;

    std::unordered_map<std::string, PackData> packDatas;

    std::vector<PackInfo> packInfos;
    std::vector<PackInfo> selectablePackInfos;

    std::unordered_map<std::string, std::string> musicPathMap;
    std::map<std::string, MusicData> musicDataMap;
    std::map<std::string, StyleData> styleDataMap;
    std::map<std::string, ProfileData> profileDataMap;
    ProfileData* currentProfilePtr{nullptr};

    std::string buf;

    template <typename... Ts>
    [[nodiscard]] std::string& concatIntoBuf(const Ts&...);

    [[nodiscard]] bool loadAllPackDatas();
    [[nodiscard]] bool loadAllPackAssets();
    [[nodiscard]] bool verifyPackDependencies();
    [[nodiscard]] bool loadAllLocalProfiles();

    [[nodiscard]] bool loadPackData(const ssvufs::Path& packPath);

    [[nodiscard]] bool loadPackAssets(const PackData& packData);

    void loadPackAssets_loadMusic(
        const std::string& mPackId, const ssvufs::Path& mPath);
    void loadPackAssets_loadMusicData(
        const std::string& mPackId, const ssvufs::Path& mPath);
    void loadPackAssets_loadStyleData(
        const std::string& mPackId, const ssvufs::Path& mPath);
    void loadPackAssets_loadLevelData(
        const std::string& mPackId, const ssvufs::Path& mPath);
    void loadPackAssets_loadCustomSounds(
        const std::string& mPackId, const ssvufs::Path& mPath);

    [[nodiscard]] std::string getCurrentLocalProfileFilePath();

private:
    LoadInfo loadInfo;

public:
    HGAssets(Steam::steam_manager* mSteamManager, bool mHeadless,
        bool mLevelsOnly = false);

    ~HGAssets();

    [[nodiscard]] LoadInfo& getLoadResults();

    [[nodiscard]] sf::Texture& getTexture(const std::string& mId);
    [[nodiscard]] sf::Texture& getTextureOrNullTexture(const std::string& mId);

    [[nodiscard]] sf::Font& getFont(const std::string& mId);
    [[nodiscard]] sf::Font& getFontOrNullFont(const std::string& mId);

    [[nodiscard]] bool isValidLevelId(
        const std::string& mLevelId) const noexcept;

    [[nodiscard]] const LevelData& getLevelData(
        const std::string& mAssetId) const;

    [[nodiscard]] bool packHasLevels(const std::string& mPackId);

    [[nodiscard]] const std::vector<std::string>& getLevelIdsByPack(
        const std::string& mPackId);

    [[nodiscard]] const std::unordered_map<std::string, PackData>&
    getPackDatas();

    [[nodiscard]] bool isValidPackId(const std::string& mPackId) const noexcept;

    [[nodiscard]] const PackData& getPackData(const std::string& mPackId);

    [[nodiscard]] const std::vector<PackInfo>&
    getSelectablePackInfos() const noexcept;

    [[nodiscard]] const PackData* findPackData(
        const std::string& mPackDisambiguator, const std::string& mPackName,
        const std::string& mPackAuthor) const noexcept;

    [[nodiscard]] const MusicData& getMusicData(
        const std::string& mPackId, const std::string& mId);
    [[nodiscard]] const StyleData& getStyleData(
        const std::string& mPackId, const std::string& mId);

    [[nodiscard]] std::string reloadPack(
        const std::string& mPackId, const std::string& mPath);
    [[nodiscard]] std::string reloadLevel(const std::string& mPackId,
        const std::string& mPath, const std::string& mId);

    [[nodiscard]] float getLocalScore(const std::string& mId);
    void setLocalScore(const std::string& mId, float mScore);

    void saveCurrentLocalProfile();
    void saveAllProfiles();

    [[nodiscard]] bool anyLocalProfileActive() const;
    [[nodiscard]] ProfileData& getCurrentLocalProfile();
    [[nodiscard]] const ProfileData& getCurrentLocalProfile() const;
    [[nodiscard]] ProfileData* getLocalProfileByName(const std::string& mName);
    [[nodiscard]] const ProfileData* getLocalProfileByName(
        const std::string& mName) const;
    [[nodiscard]] std::size_t getLocalProfilesSize();
    [[nodiscard]] std::vector<std::string> getLocalProfileNames();

    [[nodiscard]] bool pIsValidLocalProfile() const;
    [[nodiscard]] const std::string& pGetName() const;

    void pSaveCurrent();
    void pSaveAll();
    void pSetCurrent(const std::string& mName);
    void pCreate(const std::string& mName);
    void pRemove(const std::string& mName);

    [[nodiscard]] sf::SoundBuffer* getSoundBuffer(const std::string& assetId);

    [[nodiscard]] const std::string* getMusicPath(
        const std::string& assetId) const;

    [[nodiscard]] const std::unordered_map<std::string, LevelData>&
    getLevelDatas() const noexcept;
};

} // namespace hg
