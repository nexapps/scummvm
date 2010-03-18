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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 */


#ifndef TEENAGENT_PACK_H
#define TEENAGENT_PACK_H

#include "common/file.h"
#include "common/array.h"

namespace TeenAgent {

class Pack {
protected:
	uint32 _files_count;
public:
	Pack(): _files_count(0) {}
	virtual ~Pack() {};
	virtual bool open(const Common::String &filename) = 0;
	virtual void close() = 0;

	virtual uint32 files_count() const { return _files_count; }
	virtual uint32 get_size(uint32 id) const = 0;
	virtual uint32 read(uint32 id, byte *dst, uint32 size) const = 0;
	virtual Common::SeekableReadStream *getStream(uint32 id) const = 0;
};

class FilePack : public Pack {
	mutable Common::File file;
	uint32 *offsets;

public:
	FilePack();
	~FilePack();

	bool open(const Common::String &filename);
	void close();

	uint32 get_size(uint32 id) const;
	uint32 read(uint32 id, byte *dst, uint32 size) const;
	Common::SeekableReadStream *getStream(uint32 id) const;
};

class MemoryPack : public Pack {
	struct Chunk {
		byte *data;
		uint32 size;
		inline Chunk(): data(0), size(0) {}
		inline Chunk(const Chunk &c): data(c.data), size(c.size) { c.reset(); }
		inline Chunk& operator=(const Chunk &c) { data = c.data; size = c.size; c.reset(); return *this; }
		inline ~Chunk() { delete[] data; }
		inline void reset() const {
			Chunk *c = const_cast<Chunk *>(this);
			c->data = 0;
			c->size = 0;
		}
	};
	Common::Array<Chunk> chunks;

public:
	bool open(const Common::String &filename);
	void close();

	uint32 get_size(uint32 id) const;
	uint32 read(uint32 id, byte *dst, uint32 size) const;
	Common::SeekableReadStream *getStream(uint32 id) const;
};

} // End of namespace TeenAgent

#endif
