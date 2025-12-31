/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schluessel <lokoxe@gmail.com>                    *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef __GHSH_FB_SCREEN__
#define __GHSH_FB_SCREEN__

#include "screen.hpp"
#include <libdevice/interface.hpp>
#include <libvideo/videodriver.hpp>

class fb_screen_t : public screen_t
{
	g_user_mutex exitFlag;
	g_user_mutex lock;

	g_tid driverTid = 0;
	g_device_id deviceId = 0;
	g_video_mode_info mode{};

	int cursorX = 0;
	int cursorY = 0;
	int columns = 0;
	int rows = 0;

	uint32_t fgColor = 0xFFFFFFFF;
	uint32_t bgColor = 0x00000000;

	bool waitForVideoDevice(g_tid& outDriver, g_device_id& outDevice);
	bool initializeVideo();
	void clearPixels();
	void scrollUp();
	void drawChar(int x, int y, char c);
	uint32_t mapColor(screen_color_t color) const;

public:
	bool initialize(g_user_mutex exitFlag) override;
	g_key_info readInput() override;
	void clean() override;
	void backspace() override;
	void write(char c) override;
	void flush() override;
	void remove() override;
	void setCursor(int x, int y) override;
	int getCursorX() override;
	int getCursorY() override;
	void setCursorVisible(bool visible) override;
	void setScrollAreaScreen() override;
	void setScrollArea(int start, int end) override;
	void scroll(int value) override;
	int getColumns() override;
	int getRows() override;
	void setColorForeground(int c) override;
	void setColorBackground(int c) override;
};

#endif
