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
 * $URL:  $
 * $Id:  $
 */

#include "audio/softsynth/fmtowns_pc98/towns_midi.h"
#include "common/textconsole.h"

class TownsMidiOutputChannel {
friend class TownsMidiInputChannel;
public:
	TownsMidiOutputChannel(MidiDriver_TOWNS *driver, int chanId);
	~TownsMidiOutputChannel();

	void noteOn(uint8 msb, uint16 lsb);
	void noteOnAdjust(uint8 msb, uint16 lsb);
	void setupProgram(const uint8 *data, uint8 vol1, uint8 vol2);
	void noteOnSubSubSub_s1(int index, uint8 c, const uint8 *instr);
	
	void connect(TownsMidiInputChannel *chan);
	void disconnect();

	enum CheckPriorityStatus {
		kDisconnected = -3,
		kHighPriority = -2
	};

	int checkPriority(int pri);

private:
	void keyOn();
	void keyOff();
	void internKeyOnFrq(uint16 frq);
	void out(uint8 reg, uint8 val);

	TownsMidiInputChannel *_midi;
	TownsMidiOutputChannel *_prev;
	TownsMidiOutputChannel *_next;
	uint8 _fld_c;
	uint8 _chan;
	uint8 _note;
	uint8 _tl2;
	uint8 _tl1;
	uint8 _noteOffMarker;
	uint32 _duration;
	uint8 _fld_13;
	uint8 _prg;	

	uint16 _freq;
	int16 _freqAdjust;

	struct StateA {
		uint8 a[50];
	} *_stateA;

	struct StateB {
		uint8 b1;
		uint8 b2;
		uint8 b3;
		uint8 b4;
		uint8 b5;
		uint8 b6;
		uint8 b7;
		uint8 b8;
		uint8 b9;
		uint8 b10;
		uint8 b11;
	} *_stateB;

	MidiDriver_TOWNS *_driver;

	static const uint8 _chanMap[];
	static const uint8 _freqMSB[];
	static const uint16 _freqLSB[];
};

class TownsMidiInputChannel : public MidiChannel {
friend class TownsMidiOutputChannel;
public:
	TownsMidiInputChannel(MidiDriver_TOWNS *driver, int chanIndex);
	~TownsMidiInputChannel();

	MidiDriver *device() { return _driver; }
	byte getNumber() { return _chanIndex; }
	bool allocate();
	void release();

	void send(uint32 b);

	void noteOff(byte note);
	void noteOn(byte note, byte velocity);
	void programChange(byte program);
	void pitchBend(int16 bend);
	void controlChange(byte control, byte value);
	void pitchBendFactor(byte value);
	void priority(byte value);
	void sysEx_customInstrument(uint32 type, const byte *instr);

private:
	TownsMidiOutputChannel *_outChan;
	//TownsMidiInputChannel *_prev;
	//TownsMidiInputChannel *_next;
	
	uint8 *_instrument;
	uint8 _prg;
	uint8 _chanIndex;
	uint8 _effectLevel;
	uint8 _priority;
	uint8 _vol;
	uint8 _tl;
	uint8 _pan;
	uint8 _panEff;
	int8 _transpose;
	uint8 _percS;
	uint8 _fld_22;
	uint8 _pitchBendFactor;
	uint16 _freqLSB;

	bool _allocated;

	MidiDriver_TOWNS *_driver;

	static const uint8 _programAdjustLevel[];
};

TownsMidiOutputChannel::TownsMidiOutputChannel(MidiDriver_TOWNS *driver, int chanIndex) : _driver(driver), _chan(chanIndex),
	_midi(0), _prev(0), _next(0), _fld_c(0), _tl2(0), _note(0), _tl1(0), _noteOffMarker(0), _duration(0), _fld_13(0), _prg(0), _freq(0), _freqAdjust(0) {
	_stateA = new StateA[2];
	memset(_stateA, 0, 2 * sizeof(StateA));
	_stateB = new StateB[2];
	memset(_stateB, 0, 2 * sizeof(StateB));
}

TownsMidiOutputChannel::~TownsMidiOutputChannel() {
	delete[] _stateA;
	delete[] _stateB;
}

void TownsMidiOutputChannel::noteOn(uint8 msb, uint16 lsb) {
	_freq = (msb << 7) + lsb;
	_freqAdjust = 0;
	internKeyOnFrq(_freq);
}

void TownsMidiOutputChannel::noteOnAdjust(uint8 msb, uint16 lsb) {
	_freq = (msb << 7) + lsb;
	internKeyOnFrq(_freq + _freqAdjust);
}

void TownsMidiOutputChannel::setupProgram(const uint8 *data, uint8 vol1, uint8 vol2) {
	static const uint8 mul[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 12, 12, 15, 15 };
	const uint8 *pos = data;
	uint8 chan = _chanMap[_chan];	
	
	uint8 mulAmsFms1 = _driver->_chanState[chan].mulAmsFms = *pos++;
	uint8 tl1 = _driver->_chanState[chan].tl = (*pos++ | 0x3f) - vol1;
	uint8 attDec1 = _driver->_chanState[chan].attDec = !(*pos++);
	uint8 sus1 = _driver->_chanState[chan].sus = !(*pos++);
	uint8 unk1 = _driver->_chanState[chan].unk = *pos++;
	chan += 3;

	out(0x30, mul[mulAmsFms1 & 0x0f]);
	out(0x40, (tl1 & 0x3f) + 15);
	out(0x50, ((attDec1 >> 4) << 1) | ((attDec1 >> 4) & 1));
	out(0x60, ((attDec1 << 1) | (attDec1 & 1)) & 0x1f);
	out(0x70, (mulAmsFms1 & 0x20) ^ 0x20 ? ((sus1 & 0x0f) << 1) | 1: 0);
	out(0x80, sus1);

	uint8 mulAmsFms2 = _driver->_chanState[chan].mulAmsFms = *pos++;
	uint8 tl2 = _driver->_chanState[chan].tl = (*pos++ | 0x3f) - vol2;
	uint8 attDec2 = _driver->_chanState[chan].attDec = !(*pos++);
	uint8 sus2 = _driver->_chanState[chan].sus = !(*pos++);
	uint8 unk2 = _driver->_chanState[chan].unk = *pos++;

	uint8 mul2 = mul[mulAmsFms2 & 0x0f];
	tl2 = (tl2 & 0x3f) + 15;
	uint8 ar2 = ((attDec2 >> 4) << 1) | ((attDec2 >> 4) & 1);
	uint8 dec2 = ((attDec2 << 1) | (attDec2 & 1)) & 0x1f;
	uint8 sus2r = (mulAmsFms2 & 0x20) ^ 0x20 ? ((sus2 & 0x0f) << 1) | 1: 0;

	for (int i = 4; i < 16; i += 4) {
		out(0x30 + i, mul2);
		out(0x40 + i, tl2);
		out(0x50 + i, ar2);
		out(0x60 + i, dec2);
		out(0x70 + i, sus2r);
		out(0x80 + i, sus2);
	}

	uint8 t = _driver->_chanState[chan /*_chan*/ /*???*/].fgAlg = *pos;
	out(0xb0, ((t & 0x0e) << 2) | (((t & 1) << 1) + 5));
	t = mulAmsFms1 | mulAmsFms2;
	out(0xb4, 0xc0 | ((t & 0x80) >> 3) | ((t & 0x40) >> 5));
}

void TownsMidiOutputChannel::noteOnSubSubSub_s1(int index, uint8 c, const uint8 *instr) {
	StateA *a = &_stateA[index];
	StateB *b = &_stateB[index];
}

void TownsMidiOutputChannel::connect(TownsMidiInputChannel *chan) {
	if (!chan)
		return;
	_midi = chan;
	_next = chan->_outChan;
	_prev = 0;
	chan->_outChan = this;
	if (_next)
		_next->_prev = this;
}

void TownsMidiOutputChannel::disconnect() {
	keyOff();
	TownsMidiOutputChannel *p = _prev;
	TownsMidiOutputChannel *n = _next;

	if (n)
		n->_prev = p;
	if (p)
		p->_next = n;
	else
		_midi->_outChan = n;
	_midi = 0;
}

int TownsMidiOutputChannel::checkPriority(int pri) {
	if (!_midi)
		return kDisconnected;

	if (!_next && pri >= _midi->_priority)
		return _midi->_priority;

	return kHighPriority;
}

void TownsMidiOutputChannel::keyOn() {
	out(0x28, 0xf0/*0x30*/ /*???*/);
}

void TownsMidiOutputChannel::keyOff() {
	out(0x28, 0);
}

void TownsMidiOutputChannel::internKeyOnFrq(uint16 frq) {
	uint8 t = (frq << 1) >> 8;	
	frq = (_freqMSB[t] << 3) | _freqLSB[t] ;
	out(0xa4, frq >> 8);
	out(0xa0, frq & 0xff);
	out(0x28, 0);
	out(0x28, 0xf0/*0x30*/ /*???*/);
}

void TownsMidiOutputChannel::out(uint8 reg, uint8 val) {
	static const uint8 chanRegOffs[] = { 0, 1, 2, 0, 1, 2 };
	static const uint8 keyValOffs[] = { 0, 1, 2, 4, 5, 6 };

	if (reg == 0x28)
		val = (val & 0xf0) | keyValOffs[_chan];
	if (reg < 0x30)
		_driver->_intf->callback(17, 0, reg, val);
	else
		_driver->_intf->callback(17, _chan / 3, (reg & ~3) | chanRegOffs[_chan], val);
}

const uint8 TownsMidiOutputChannel::_chanMap[] = {
	0, 1, 2, 8, 9, 10
};

const uint8 TownsMidiOutputChannel::_freqMSB[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x80, 0x81, 0x83, 0x85,
	0x87, 0x88, 0x8A, 0x8C, 0x8E, 0x8F, 0x91, 0x93, 0x95, 0x96, 0x98, 0x9A,
	0x9C, 0x9E, 0x9F, 0xA1, 0xA3, 0xA5, 0xA6, 0xA8, 0xAA, 0xAC, 0xAD, 0xAF,
	0xB1, 0xB3, 0xB4, 0xB6, 0xB8, 0xBA, 0xBC, 0xBD, 0xBF, 0xC1, 0xC3, 0xC4,
	0xC6, 0xC8, 0xCA, 0xCB, 0xCD, 0xCF, 0xD1, 0xD2, 0xD4, 0xD6, 0xD8, 0xDA,
	0xDB, 0xDD, 0xDF, 0xE1, 0xE2, 0xE4, 0xE6, 0xE8, 0xE9, 0xEB, 0xED, 0xEF
};

const uint16 TownsMidiOutputChannel::_freqLSB[] = {
	0x02D6, 0x02D6, 0x02D6, 0x02D6, 0x02D6, 0x02D6, 0x02D6, 0x02D6,
	0x02D6, 0x02D6, 0x02D6, 0x02D6, 0x02D6, 0x02D6, 0x0301, 0x032F,
	0x0360, 0x0393, 0x03C9, 0x0403, 0x0440, 0x0481, 0x04C6, 0x050E,
	0x055B, 0x02D6, 0x0301, 0x032F, 0x0360, 0x0393, 0x03C9, 0x0403,
	0x0440, 0x0481, 0x04C6, 0x050E, 0x055B, 0x02D6, 0x0301, 0x032F,
	0x0360, 0x0393, 0x03C9, 0x0403, 0x0440, 0x0481, 0x04C6, 0x050E,
	0x055B, 0x02D6, 0x0301, 0x032F, 0x0360, 0x0393, 0x03C9, 0x0403,
	0x0440, 0x0481, 0x04C6, 0x050E, 0x055B, 0x02D6, 0x0301, 0x032F,
	0x0360, 0x0393, 0x03C9, 0x0403, 0x0440, 0x0481, 0x04C6, 0x050E,
	0x055B, 0x02D6, 0x0301, 0x032F, 0x0360, 0x0393, 0x03C9, 0x0403,
	0x0440, 0x0481, 0x04C6, 0x050E, 0x055B, 0x02D6, 0x0301, 0x032F,
	0x0360, 0x0393, 0x03C9, 0x0403, 0x0440, 0x0481, 0x04C6, 0x050E,
	0x055B, 0x055B, 0x055B, 0x055B, 0x055B, 0x055B, 0x055B, 0x055B,
	0x055B, 0x055B, 0x055B, 0x055B, 0x055B, 0x055B, 0x055B, 0x055B,
	0x055B, 0x055B, 0x055B, 0x055B, 0x055B, 0x055B, 0x055B, 0x055B,
	0x055B, 0x055B, 0x055B, 0x055B, 0x055B, 0x055B, 0x055B, 0x055B
};

TownsMidiInputChannel::TownsMidiInputChannel(MidiDriver_TOWNS *driver, int chanIndex) : MidiChannel(), _driver(driver), _outChan(0), _prg(0), _chanIndex(chanIndex),
	_effectLevel(0), _priority(0), _vol(0), _tl(0), _pan(0), _panEff(0), _transpose(0), _percS(0), _pitchBendFactor(0), _fld_22(0), _freqLSB(0), _allocated(false) {
	_instrument = new uint8[30];
	memset(_instrument, 0, 30);
}

TownsMidiInputChannel::~TownsMidiInputChannel() {
	delete _instrument;
}

bool TownsMidiInputChannel::allocate() {
	if (_allocated)
		return false;
	_allocated = true;
	return true;
}

void TownsMidiInputChannel::release() {
	_allocated = false;
}

void TownsMidiInputChannel::send(uint32 b) {
	_driver->send(b | _chanIndex);
}

void TownsMidiInputChannel::noteOff(byte note) {
	if (!_outChan)
		return;

	if (_outChan->_note != note)
		return;

	if (_fld_22)
		_outChan->_noteOffMarker = 1;
	else
		_outChan->disconnect();
}

void TownsMidiInputChannel::noteOn(byte note, byte velocity) {
	TownsMidiOutputChannel *oc = _driver->allocateOutputChannel(_priority);
	
	if (!oc)
		return;

	oc->connect(this);

	oc->_fld_c = _instrument[10] & 1;
	oc->_note = note;
	oc->_noteOffMarker = 0;
	oc->_duration = _instrument[29] * 72;
	
	oc->_tl1 = (_instrument[1] & 0x3f) + _driver->_chanOutputLevel[((velocity >> 1) << 5) + (_instrument[4] >> 2)];
	if (oc->_tl1 > 63)
		oc->_tl1 = 63;

	oc->_tl2 = (_instrument[6] & 0x3f) + _driver->_chanOutputLevel[((velocity >> 1) << 5) + (_instrument[9] >> 2)];
	if (oc->_tl2 > 63)
		oc->_tl2 = 63;

	oc->setupProgram(_instrument, oc->_fld_c == 1 ? _programAdjustLevel[_driver->_chanOutputLevel[(_tl >> 2) + (oc->_tl1 << 5)]] : oc->_tl1, _programAdjustLevel[_driver->_chanOutputLevel[(_tl >> 2) + (oc->_tl2 << 5)]]);
	oc->noteOn(note + _transpose, _freqLSB);

	if (_instrument[11] & 0x80)
		oc->noteOnSubSubSub_s1(0, _instrument[11], &_instrument[12]);
	else
		oc->_stateA[0].a[0] = 0;

	if (_instrument[20] & 0x80)
		oc->noteOnSubSubSub_s1(1, _instrument[20], &_instrument[21]);
	else
		oc->_stateA[1].a[0] = 0;	
}

void TownsMidiInputChannel::programChange(byte program) {

}

void TownsMidiInputChannel::pitchBend(int16 bend) {

}

void TownsMidiInputChannel::controlChange(byte control, byte value) {

}

void TownsMidiInputChannel::pitchBendFactor(byte value) {

}

void TownsMidiInputChannel::priority(byte value) {
	_priority = value;
}

void TownsMidiInputChannel::sysEx_customInstrument(uint32 type, const byte *instr) {
	memcpy(_instrument, instr, 30);
}

const uint8 TownsMidiInputChannel::_programAdjustLevel[] = {
	0x00, 0x04, 0x07, 0x0B, 0x0D, 0x10, 0x12, 0x14,
	0x16, 0x18, 0x1A, 0x1B, 0x1D, 0x1E, 0x1F, 0x21,
	0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
	0x2A, 0x2B, 0x2C, 0x2C, 0x2D, 0x2E, 0x2F, 0x2F,
	0x30, 0x31, 0x31, 0x32, 0x33, 0x33, 0x34, 0x35,
	0x35, 0x36, 0x36, 0x37, 0x37, 0x38, 0x38, 0x39,
	0x39, 0x3A, 0x3A, 0x3B, 0x3B, 0x3C, 0x3C, 0x3C,
	0x3D, 0x3D, 0x3E, 0x3E, 0x3E, 0x3F, 0x3F, 0x3F
};

MidiDriver_TOWNS::MidiDriver_TOWNS(Audio::Mixer *mixer) : _timerBproc(0), _timerBpara(0), _open(false) {
	_intf = new TownsAudioInterface(mixer, this);

	_channels = new TownsMidiInputChannel*[32];
	for (int i = 0; i < 32; i++)
		_channels[i] = new TownsMidiInputChannel(this, i);
	
	_out = new TownsMidiOutputChannel*[6];
	for (int i = 0; i < 6; i++)
		_out[i] = new TownsMidiOutputChannel(this, i);

	_chanState = new ChanState[32];
	memset(_chanState, 0, 32 * sizeof(ChanState));

	_chanOutputLevel = new uint8[2048];
	for (int i = 0; i < 64; i++) {
		for (int ii = 0; ii < 32; ii++)
			_chanOutputLevel[(i << 5) + ii] = ((i * (ii + 1)) >> 5) & 0xff;
	}
	for (int i = 0; i < 64; i++)
		_chanOutputLevel[i << 5] = 0;

	_tickCounter = 0;
	_curChan = 0;
}

MidiDriver_TOWNS::~MidiDriver_TOWNS() {
	close();
	delete _intf;
	setTimerCallback(0, 0);

	for (int i = 0; i < 32; i++)
		delete _channels[i];
	delete[] _channels;

	for (int i = 0; i < 6; i++)
		delete _out[i];
	delete[] _out;

	delete[] _chanState;
	delete[] _chanOutputLevel;
}

int MidiDriver_TOWNS::open() {
	if (_open)
		return MERR_ALREADY_OPEN;

	if (!_intf->init())
		return MERR_CANNOT_CONNECT;

	_intf->callback(0);

	_intf->callback(21, 255, 1);
	_intf->callback(21, 0, 1);
	_intf->callback(22, 255, 221);

	_intf->callback(33, 8);
	_intf->setSoundEffectChanMask(~0x3f);

	_open = true;

	return 0;
}

void MidiDriver_TOWNS::close() {
	_open = false;
}

void MidiDriver_TOWNS::send(uint32 b) {
	byte param2 = (b >> 16) & 0xFF;
	byte param1 = (b >> 8) & 0xFF;
	byte cmd = b & 0xF0;

	/*AdLibPart *part;
	if (chan == 9)
		part = &_percussion;
	else**/
	TownsMidiInputChannel *c = _channels[b & 0x0F];

	switch (cmd) {
	case 0x80:
		c->noteOff(param1);
		break;
	case 0x90:
		if (param2)
			c->noteOn(param1, param2);
		else
			c->noteOff(param1);
		break;
	case 0xB0:
		// supported: 1, 7, 0x40
		c->controlChange(param1, param2);
		break;
	case 0xC0:
		c->programChange(param1);
		break;
	case 0xE0:
		//part->pitchBend((param1 | (param2 << 7)) - 0x2000);
		c->pitchBend((param1 | (param2 << 7)) - 0x2000);
		break;
	case 0xF0:
		warning("MidiDriver_TOWNS: Receiving SysEx command on a send() call");
		break;

	default:
		break;
	}
}

void MidiDriver_TOWNS::setTimerCallback(void *timer_param, Common::TimerManager::TimerProc timer_proc) {
	_timerBproc = timer_proc;
	_timerBpara = timer_param;
}

uint32 MidiDriver_TOWNS::getBaseTempo() {
	return 4167;
}

MidiChannel *MidiDriver_TOWNS::allocateChannel() {
	for (int i = 0; i < 32; ++i) {		
		TownsMidiInputChannel *chan = _channels[i];
		if (chan->allocate())
			return chan;
	}

	return 0;
}

MidiChannel *MidiDriver_TOWNS::getPercussionChannel() {
	return 0;
}

void MidiDriver_TOWNS::timerCallback(int timerId) {
	if (!_open)
		return;

	switch (timerId) {
	case 1:
		if (_timerBproc) {
			_timerBproc(_timerBpara);
			_tickCounter += 10000;
			while (_tickCounter >= 4167) {
				_tickCounter -= 4167;
				//_timerBproc(_timerBpara);
			}
		}
		break;
	default:
		break;
	}
}

TownsMidiOutputChannel *MidiDriver_TOWNS::allocateOutputChannel(int pri) {
	TownsMidiOutputChannel *res = 0;

	for (int i = 0; i < 6; i++) {
		if (++_curChan == 6)
			_curChan = 0;

		int s = _out[i]->checkPriority(pri);
		if (s == TownsMidiOutputChannel::kDisconnected)
			return _out[i];

		if (s != TownsMidiOutputChannel::kHighPriority) {
			pri = s;
			res = _out[i];
		}
	}
	
	if (res)
		res->disconnect();

	return res;
}
