// Copyright (c) 2013 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: http://opensource.org/licenses/AFL-3.0

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <SFML/Audio.hpp>
#include <SSVStart.h>
#include "Components/CPlayer.h"
#include "Components/CWall.h"
#include "Data/StyleData.h"
#include "Global/Assets.h"
#include "Global/Config.h"
#include "Global/Factory.h"
#include "Utils/Utils.h"
#include "HexagonGame.h"
#include "MenuGame.h"

using namespace std;
using namespace sf;
using namespace ssvs;
using namespace ssvs::Utils;
using namespace sses;

namespace hg
{
	HexagonGame::HexagonGame(GameWindow& mGameWindow) : window(mGameWindow)
	{
		flashPolygon.clear();
		flashPolygon.append({{-100.f, -100.f}, Color{255, 255, 255, 0}});
		flashPolygon.append({{getWidth() + 100.f, -100.f}, Color{255, 255, 255, 0}});
		flashPolygon.append({{getWidth() + 100.f, getHeight() + 100.f}, Color{255, 255, 255, 0}});
		flashPolygon.append({{-100.f, getHeight() + 100.f}, Color{255, 255, 255, 0}});

		game.onUpdate += [&](float mFrameTime) { update(mFrameTime); };

		game.onDraw += [&](){ window.clear(Color::Black); };
		game.onDraw += [&](){ backgroundCamera.apply(); };

		if(!getNoBackground()) game.onDraw += [&](){ styleData.drawBackground(window.getRenderWindow(), {0, 0}, getSides()); };

		game.onDraw += [&](){ manager.draw(); };
		game.onDraw += [&](){ overlayCamera.apply(); };
		game.onDraw += [&](){ drawText(); };
		game.onDraw += [&](){ drawOnWindow(flashPolygon); };
	}

	void HexagonGame::newGame(string mId, bool mFirstPlay, float mDifficultyMult)
	{
		firstPlay = mFirstPlay;
		setLevelData(getLevelData(mId), mFirstPlay);
		difficultyMult = mDifficultyMult;

		// Audio cleanup
		stopAllSounds();
		playSound("go.ogg");
		stopLevelMusic();
		playLevelMusic();

		// Events cleanup
		clearMessage();

		for(auto eventPtr : eventPtrs) delete eventPtr;
		eventPtrs.clear();

		while(!eventPtrQueue.empty()) { delete eventPtrQueue.front(); eventPtrQueue.pop(); }
		eventPtrQueue = queue<EventData*>{};

		// Game status cleanup
		hgStatus = {};
		restartId = mId;
		restartFirstTime = false;
		setSides(levelData.getSides());
		backgroundCamera.setRotation(0);

		// Manager cleanup
		manager.clear();
		createPlayer(manager, this, {0, 0});

		// Timeline cleanup
		timeline = Timeline{};
		messageTimeline = Timeline{};
		effectTimelineManager.clear();

		// LUA context cleanup
		if(!mFirstPlay) runLuaFunction<void>("onUnload");
		lua = Lua::LuaContext{};
		initLua();
		runLuaFile(levelData.getValueString("lua_file"));
		runLuaFunction<void>("onLoad");

		// Random rotation direction
		if(getRnd(0, 100) > 50) setRotationSpeed(getRotationSpeed() * -1);

		// Reset zoom
		backgroundCamera.setView({{0, 0}, {getWidth() * getZoomFactor(), getHeight() * getZoomFactor()}});
	}
	void HexagonGame::death()
	{
		playSound("death.ogg");
		playSound("gameOver.ogg");

		if(getInvincible()) return;

		hgStatus.flashEffect = 255;
		shakeCamera(effectTimelineManager, overlayCamera);
		shakeCamera(effectTimelineManager, backgroundCamera);
		hgStatus.hasDied = true;
		stopLevelMusic();
		checkAndSaveScore();
	}

	void HexagonGame::incrementDifficulty()
	{
		playSound("levelUp.ogg");

		setRotationSpeed(levelData.getRotationSpeed() + levelData.getRotationSpeedIncrement() * getSign(getRotationSpeed()));
		setRotationSpeed(levelData.getRotationSpeed() * -1);
		
		if(hgStatus.fastSpin < 0 && abs(getRotationSpeed()) > levelData.getValueFloat("rotation_speed_max"))
			setRotationSpeed(levelData.getValueFloat("rotation_speed_max") * getSign(getRotationSpeed()));

		hgStatus.fastSpin = levelData.getFastSpin();
		timeline.insert<Do>(timeline.getCurrentIndex() + 1, [&]{ sideChange(getRnd(levelData.getSidesMin(), levelData.getSidesMax() + 1)); });
	}
	void HexagonGame::sideChange(int mSideNumber)
	{
		if(manager.getComponents("wall").size() > 0)
		{
			timeline.insert<Do>(timeline.getCurrentIndex() + 1, [&]{ clearAndResetTimeline(timeline); });
			timeline.insert<Do>(timeline.getCurrentIndex() + 1, [&, mSideNumber]{ sideChange(mSideNumber); });
			timeline.insert<Wait>(timeline.getCurrentIndex() + 1, 1);
			return;
		}

		runLuaFunction<void>("onIncrement");
		setSpeedMultiplier(levelData.getSpeedMultiplier() + levelData.getSpeedIncrement());
		setDelayMultiplier(levelData.getDelayMultiplier() + levelData.getDelayIncrement());

		if(hgStatus.randomSideChangesEnabled) setSides(mSideNumber);
	}

	void HexagonGame::checkAndSaveScore()
	{
		string validator{getScoreValidator(levelData.getId(), difficultyMult)};
		if(getScore(validator) < hgStatus.currentTime) setScore(validator, hgStatus.currentTime);
		saveCurrentProfile();
	}
	void HexagonGame::goToMenu()
	{
		stopAllSounds();
		playSound("beep.ogg");

		checkAndSaveScore();
		runLuaFunction<void>("onUnload");
		window.setGame(&mgPtr->getGame());
		mgPtr->init();
	}
	void HexagonGame::changeLevel(string mId, bool mFirstTime)
	{
		checkAndSaveScore();		
		newGame(mId, mFirstTime, difficultyMult);
	}
	void HexagonGame::addMessage(string mMessage, float mDuration)
	{
		Text* text = new Text(mMessage, getFont("imagine.ttf"), 40 / getZoomFactor());
		text->setPosition(Vector2f(getWidth() / 2, getHeight() / 6));
		text->setOrigin(text->getGlobalBounds().width / 2, 0);

		messageTimeline.append<Do>([&, text, mMessage]{ playSound("beep.ogg"); messageTextPtr = text; });
		messageTimeline.append<Wait>(mDuration);
		messageTimeline.append<Do>([=]{ messageTextPtr = nullptr; delete text; });
	}
	void HexagonGame::clearMessage()
	{
		if(messageTextPtr == nullptr) return;

		delete messageTextPtr;
		messageTextPtr = nullptr;
	}

	void HexagonGame::setLevelData(LevelData mLevelSettings, bool mMusicFirstPlay)
	{
		levelData = mLevelSettings;
		styleData = getStyleData(levelData.getStyleId());
		musicData = getMusicData(levelData.getMusicId());
		musicData.setFirstPlay(mMusicFirstPlay);
	}

	bool HexagonGame::isKeyPressed(Keyboard::Key mKey) 		{ return window.isKeyPressed(mKey); }
	bool HexagonGame::isButtonPressed(Mouse::Button mButton){ return window.isButtonPressed(mButton); }
	void HexagonGame::playLevelMusic() { if(!getNoMusic()) musicData.playRandomSegment(musicPtr); }
	void HexagonGame::stopLevelMusic() { if(!getNoMusic()) if(musicPtr != nullptr) musicPtr->stop(); }

	void HexagonGame::wall(int mSide, float mThickness)
	{
		createWall(manager, this, {0, 0}, mSide, mThickness, baseSpeed, getSpeedMultiplier());
	}
	void HexagonGame::wallAdj(int mSide, float mThickness, float mSpeedAdj)
	{
		createWall(manager, this, {0, 0}, mSide, mThickness, baseSpeed * mSpeedAdj, getSpeedMultiplier());
	}
}
