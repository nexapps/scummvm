/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "common/events.h"
#include "common/file.h"
#include "common/savefile.h"
#include "common/config-manager.h"

#include "base/plugins.h"
#include "base/version.h"

#include "graphics/cursorman.h"

#include "sound/mididrv.h"
#include "sound/mixer.h"

#include "agi/preagi.h"
#include "agi/graphics.h"
#include "agi/sprite.h"
#include "agi/opcodes.h"
#include "agi/keyboard.h"
#include "agi/menu.h"
#include "agi/sound.h"

// preagi engines
#include "agi/preagi_mickey.h"
#include "agi/preagi_troll.h"
#include "agi/preagi_winnie.h"

namespace Agi {

PreAgiEngine::PreAgiEngine(OSystem *syst, const AGIGameDescription *gameDesc) : AgiBase(syst, gameDesc) {

	// Setup mixer
	if (!_mixer->isReady()) {
		warning("Sound initialization failed.");
	}

	_mixer->setVolumeForSoundType(Audio::Mixer::kSFXSoundType, ConfMan.getInt("sfx_volume"));
	_mixer->setVolumeForSoundType(Audio::Mixer::kMusicSoundType, ConfMan.getInt("music_volume"));

	/*
	const GameSettings *g;

	const char *gameid = ConfMan.get("gameid").c_str();
	for (g = agiSettings; g->gameid; ++g)
		if (!scumm_stricmp(g->gameid, gameid))
			_gameId = g->id;
	*/

	_rnd = new Common::RandomSource();

	Common::addSpecialDebugLevel(kDebugLevelMain, "Main", "Generic debug level");
	Common::addSpecialDebugLevel(kDebugLevelResources, "Resources", "Resources debugging");
	Common::addSpecialDebugLevel(kDebugLevelSprites, "Sprites", "Sprites debugging");
	Common::addSpecialDebugLevel(kDebugLevelInventory, "Inventory", "Inventory debugging");
	Common::addSpecialDebugLevel(kDebugLevelInput, "Input", "Input events debugging");
	Common::addSpecialDebugLevel(kDebugLevelMenu, "Menu", "Menu debugging");
	Common::addSpecialDebugLevel(kDebugLevelScripts, "Scripts", "Scripts debugging");
	Common::addSpecialDebugLevel(kDebugLevelSound, "Sound", "Sound debugging");
	Common::addSpecialDebugLevel(kDebugLevelText, "Text", "Text output debugging");
	Common::addSpecialDebugLevel(kDebugLevelSavegame, "Savegame", "Saving & restoring game debugging");

	memset(&_game, 0, sizeof(struct AgiGame));
	memset(&_debug, 0, sizeof(struct AgiDebug));
	memset(&g_mouse, 0, sizeof(struct Mouse));

/*
	_game.clockEnabled = false;
	_game.state = STATE_INIT;

	_keyQueueStart = 0;
	_keyQueueEnd = 0;

	_keyControl = 0;
	_keyAlt = 0;

	_allowSynthetic = false;

	g_tickTimer = 0;

	_intobj = NULL;

	_stackSize = 0;
	_imageStack = NULL;
	_imageStackPointer = 0;

	_lastSentence[0] = 0;
	memset(&_stringdata, 0, sizeof(struct StringData));

	_objects = NULL;

	_oldMode = -1;

	_firstSlot = 0;
*/
}

void PreAgiEngine::initialize() {
	// TODO: Some sound emulation modes do not fit our current music
	//       drivers, and I'm not sure what they are. For now, they might
	//       as well be called "PC Speaker" and "Not PC Speaker".

	switch (MidiDriver::detectMusicDriver(MDT_PCSPK)) {
	case MD_PCSPK:
		_soundemu = SOUND_EMU_PC;
		break;
	default:
		_soundemu = SOUND_EMU_NONE;
		break;
	}

	if (ConfMan.hasKey("render_mode")) {
		_renderMode = Common::parseRenderMode(ConfMan.get("render_mode").c_str());
	} else if (ConfMan.hasKey("platform")) {
		switch (Common::parsePlatform(ConfMan.get("platform"))) {
		case Common::kPlatformAmiga:
			_renderMode = Common::kRenderAmiga;
			break;
		case Common::kPlatformPC:
			_renderMode = Common::kRenderEGA;
			break;
		default:
			_renderMode = Common::kRenderEGA;
			break;
		}
	}

	_gfx = new GfxMgr(this);
	_sound = new SoundMgr(this, _mixer);
	_picture = new PictureMgr(this, _gfx);
	//_sprites = new SpritesMgr(this, _gfx);

	_gfx->initMachine();

	_game.gameFlags = 0;

	_game.colorFg = 15;
	_game.colorBg = 0;

	_defaultColor = 0xF;

	_game.name[0] = '\0';

	_game.sbufOrig = (uint8 *)calloc(_WIDTH, _HEIGHT * 2); // Allocate space for two AGI screens vertically
	_game.sbuf16c  = _game.sbufOrig + SBUF16_OFFSET; // Make sbuf16c point to the 16 color (+control line & priority info) AGI screen
	_game.sbuf     = _game.sbuf16c; // Make sbuf point to the 16 color (+control line & priority info) AGI screen by default

	_game.lineMinPrint = 0; // hardcoded

	_gfx->initVideo();
	_sound->initSound();

	//_timer->installTimerProc(agiTimerFunctionLow, 10 * 1000, NULL);

	_game.ver = -1;		// Don't display the conf file warning

	debugC(2, kDebugLevelMain, "Detect game");

	/* clear all resources and events */
	for (int i = 0; i < MAX_DIRS; i++) {
		memset(&_game.pictures[i], 0, sizeof(struct AgiPicture));
		memset(&_game.sounds[i], 0, sizeof(class AgiSound *)); // _game.sounds contains pointers now
		memset(&_game.dirPic[i], 0, sizeof(struct AgiDir));
		memset(&_game.dirSound[i], 0, sizeof(struct AgiDir));
	}

	debugC(2, kDebugLevelMain, "Init sound");
}

PreAgiEngine::~PreAgiEngine() {

}

int PreAgiEngine::init() {
	// Initialize backend
	_system->beginGFXTransaction();
	initCommonGFX(false);
	_system->initSize(320, 200);
	_system->endGFXTransaction();

	initialize();

	_gfx->gfxSetPalette();

	return 0;
}

int PreAgiEngine::go() {
	setflag(fSoundOn, true);	// enable sound

	// run preagi engine main loop
	switch (getGameID()) {
		case GID_MICKEY:
			{
				Mickey *mickey = new Mickey(this);
				mickey->init();
				mickey->run();
			}
			break;
		case GID_WINNIE:
			{
				Winnie *winnie = new Winnie(this);
				winnie->init();
				winnie->run();
			}
			break;
		case GID_TROLL:
			{
				Troll *troll = new Troll(this);
				troll->init();
				troll->run();
			}
			break;
		default:
			error("Unknown preagi engine");
			break;
	}
	return 0;
}

} // End of namespace Agi
