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

#ifndef M4_MADS_MENUS_H
#define M4_MADS_MENUS_H

#include "common/str-array.h"
#include "m4/viewmgr.h"
#include "m4/font.h"

namespace M4 {

#define MADS_MENU_ANIM_DELAY 70

enum MadsGameAction {START_GAME, RESUME_GAME, SHOW_INTRO, CREDITS, QUOTES, EXIT};

class RexMainMenuView : public View {
private:
	M4Surface *_bgSurface;
	RGBList *_bgPalData;
	int _menuItemIndex;
	int _frameIndex;
	bool _skipFlag;
	SpriteAsset *_menuItem;
	Common::Array<RGBList *> _itemPalData;
	uint32 _delayTimeout;
	int _highlightedIndex;

	int getHighlightedItem(int x, int y);
	void handleAction(MadsGameAction action);
public:
	RexMainMenuView(MadsM4Engine *vm);
	~RexMainMenuView();

	bool onEvent(M4EventType eventType, int32 param, int x, int y, bool &captureEvents);
	void updateState();
};

class DragonMainMenuView : public View {
private:
	//M4Surface *_bgSurface;
	RGBList *_bgPalData;
	int _menuItemIndex;
	int _frameIndex;
	bool _skipFlag;
	SpriteAsset *_menuItem;
	Common::Array<RGBList *> _itemPalData;
	uint32 _delayTimeout;
	int _highlightedIndex;

	int getHighlightedItem(int x, int y);
	void handleAction(MadsGameAction action);
public:
	DragonMainMenuView(MadsM4Engine *vm);
	~DragonMainMenuView();

	bool onEvent(M4EventType eventType, int32 param, int x, int y, bool &captureEvents);
	void updateState();
};

class MadsMainMenuView : public View {
public:
	MadsMainMenuView(MadsM4Engine *vm);

	bool onEvent(M4EventType eventType, int32 param, int x, int y, bool &captureEvents);
	void updateState();
};

class DialogTextEntry {
public:
	bool in_use;
	int16 field_2;
	Common::Point pos;
	char text[80];
	Font *font;
	int widthAdjust;

	int textDisplay_index;

	DialogTextEntry() { in_use = false; };
};

#define DIALOG_LINES_SIZE 20

enum MadsTextAlignment { ALIGN_CENTER = -1, ALIGN_CHAR_CENTER = -2, RIGHT_ALIGN = -3 };


class RexDialogView: public MadsView {
private:
	int _priorSceneId;

	void initialiseLines();
	void initialiseGraphics();
	void loadBackground();
	void loadMenuSprites();
protected:
	int _word_8502C;
	int _selectedLine;
	int _lineIndex;
	bool _enterFlag;
	Common::StringArray _textLines;

	void setFrame(int frameNumber, int depth);
	void initVars();
	void addLine(const char *msg_p, Font *font, MadsTextAlignment alignment, int left, int top);
	void addQuote(Font *font, MadsTextAlignment alignment, int left, int top, int id1, int id2 = 0);
protected:
	M4Surface *_backgroundSurface;
	RGBList *_bgPalData;
	SpriteAsset *_menuSprites;
	RGBList *_spritesPalData;

	Common::Array<DialogTextEntry> _dialogText;
	int _totalTextEntries;
	int _dialogSelectedLine;
	Common::StringArray _saveList;
public:
	RexDialogView();
	~RexDialogView();

	virtual void updateState();
	virtual void onRefresh(RectList *rects, M4Surface *destSurface);
};

class RexGameMenuDialog: public RexDialogView {
private:
	void addLines();
public:
	RexGameMenuDialog();

	virtual void onRefresh(RectList *rects, M4Surface *destSurface);
};

}

#endif
