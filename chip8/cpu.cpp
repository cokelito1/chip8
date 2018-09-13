#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

#include "cpu.h"

Cpu::Cpu() {
	for (unsigned int i = 0; i < 16; i++) {
		Stack[i] = 0x00;
		Keyboard[i] = 0x00;
		RegisterBank.V[i] = 0x00;
	}

	for (unsigned int i = 0; i < (32 * 64); i++) {
		Framebuffer[i] = 0;
	}

	for (unsigned int i = 0; i < Memory.size(); i++) {
		Memory[i] = 0x00;
	}

	for (unsigned int i = 0; i < 80; i++) {
		Memory[i] = Font[i];
	}

	RegisterBank.I = 0x00;
	RegisterBank.PC = 0x200;
	RegisterBank.SP = 0x00;

	window = new sf::RenderWindow(sf::VideoMode(800, 600), "Chip-8");
	sf::View view = window->getView();
	view.setCenter(width / 2, height / 2);

	view.setSize(sf::Vector2f(width, height));
	window->setView(view);

	window->setFramerateLimit(250);

	frameTexture.create(width, height);
	frame.setTexture(frameTexture);

	sf::Int16 raw[44100];
	if (!ToneBuffer.loadFromSamples(raw, 44100, 1, 44100)) {
		std::cerr << "Error al cargar samples" << std::endl;
		exit(0);
	}
	Tone.setBuffer(ToneBuffer);
	Tone.setLoop(true);

	std::cout << "Window " << window->getSize().x << " " << window->getSize().y << std::endl;
	std::cout << "View " << view.getSize().x << " " << view.getSize().y << std::endl;
	std::cout << "Window View " << window->getView().getSize().x << " " << window->getView().getSize().y << std::endl;

	RegisterBank.SoundTimer = 0;
	RegisterBank.DelayTimer = 0;
}

Cpu::Cpu(const std::string& RomfilePath) : Cpu() {
	if (!LoadRom(RomfilePath)) {
		std::cerr << "Error al abrir " << RomfilePath << std::endl;
		exit(0);
	}
}

Cpu::~Cpu() {
	delete window;
}

void Cpu::Reset() {
	for (unsigned int i = 0; i < 16; i++) {
		Stack[i] = 0x00;
		Keyboard[i] = 0x00;
		RegisterBank.V[i] = 0x00;
	}

	for (unsigned int i = 0; i < (32 * 64); i++) {
		Framebuffer[i] = 0;
	}

	for (unsigned int i = 0; i < Memory.size(); i++) {
		Memory[i] = 0x00;
	}

	for (unsigned int i = 0; i < 80; i++) {
		Memory[i] = Font[i];
	}

	RegisterBank.I = 0x00;
	RegisterBank.PC = 0x200;
	RegisterBank.SP = 0x00;

	if (!LoadRom(RomPath)) {
		std::cerr << "Error al abrir " << RomPath << std::endl;
		exit(0);
	}

	RegisterBank.SoundTimer = 0;
	RegisterBank.DelayTimer = 0;

	Start();
}

bool Cpu::LoadRom(const std::string& RomfilePath) {
	RomPath = RomfilePath;
	std::ifstream Rom(RomfilePath, std::ios::in | std::ios::binary | std::ios::ate);
	if (!Rom.is_open()) {
		return false;
	}

	size_t FileSize = Rom.tellg();
	if (FileSize > (0x1000 - 0x200)) {
		std::cerr << "Rom muy grande" << std::endl;
		return false;
	}
	Rom.seekg(0);

	uint8_t *RomBuffer = new uint8_t[FileSize];
	Rom.read((char *) RomBuffer, FileSize);
	Rom.close();

	for (unsigned int i = 0; i < FileSize; i++) {
		Memory[0x200 + i] = RomBuffer[i];
	}

	delete RomBuffer;
	return true;
}

void Cpu::Start() {

	int counter = 0;
	while (RegisterBank.PC < Memory.size() && window->isOpen()) {
		sf::Uint8 FrameBufferSFML[(32 * 64) * 4];
		
		int PixelOffsetCounter = 0;
		for (int i = 0; i < (32 * 64); i++) {
			if (Framebuffer[i] == 1) {
				FrameBufferSFML[PixelOffsetCounter] = 0xFF;
				FrameBufferSFML[PixelOffsetCounter + 1] = 0xFF;
				FrameBufferSFML[PixelOffsetCounter + 2] = 0xFF;
				FrameBufferSFML[PixelOffsetCounter + 3] = 0xFF;
			}
			else {
				FrameBufferSFML[PixelOffsetCounter] = 0x00;
				FrameBufferSFML[PixelOffsetCounter + 1] = 0x00;
				FrameBufferSFML[PixelOffsetCounter + 2] = 0x00;
				FrameBufferSFML[PixelOffsetCounter + 3] = 0x00;
			}
			PixelOffsetCounter += 4;
		}
		frameTexture.update(FrameBufferSFML, width, height, 0, 0);
		
		sf::Event evt;
		while (window->pollEvent(evt)) {
			switch (evt.type) {
			case sf::Event::Closed:
				window->close();
				break;
			case sf::Event::KeyPressed:
				switch (evt.key.code)
				{
				case sf::Keyboard::Num1:
					Keyboard[0] = 1;
					break;
				case sf::Keyboard::Num2:
					Keyboard[1] = 1;
					break;
				case sf::Keyboard::Num3:
					Keyboard[2] = 1;
					break;
				case sf::Keyboard::Num4:
					Keyboard[3] = 1;
					break;
				case sf::Keyboard::Q:
					Keyboard[4] = 1;
					break;
				case sf::Keyboard::W:
					Keyboard[5] = 1;
					break;
				case sf::Keyboard::E:
					Keyboard[6] = 1;
					break;
				case sf::Keyboard::R:
					Keyboard[7] = 1;
					break;
				case sf::Keyboard::A:
					Keyboard[8] = 1;
					break;
				case sf::Keyboard::S:
					Keyboard[9] = 1;
					break;
				case sf::Keyboard::D:
					Keyboard[10] = 1;
					break;
				case sf::Keyboard::F:
					Keyboard[11] = 1;
					break;
				case sf::Keyboard::Z:
					Keyboard[12] = 1;
					break;
				case sf::Keyboard::X:
					Keyboard[13] = 1;
					break;
				case sf::Keyboard::C:
					Keyboard[14] = 1;
					break;
				case sf::Keyboard::V:
					Keyboard[15] = 1;
					break;
				case sf::Keyboard::P:
					Reset();
					break;
				}
				break;
			case sf::Event::KeyReleased:
				switch (evt.key.code)
				{
				case sf::Keyboard::Num1:
					Keyboard[0] = 0;
					break;
				case sf::Keyboard::Num2:
					Keyboard[1] = 0;
					break;
				case sf::Keyboard::Num3:
					Keyboard[2] = 0;
					break;
				case sf::Keyboard::Num4:
					Keyboard[3] = 0;
					break;
				case sf::Keyboard::Q:
					Keyboard[4] = 0;
					break;
				case sf::Keyboard::W:
					Keyboard[5] = 0;
					break;
				case sf::Keyboard::E:
					Keyboard[6] = 0;
					break;
				case sf::Keyboard::R:
					Keyboard[7] = 0;
					break;
				case sf::Keyboard::A:
					Keyboard[8] = 0;
					break;
				case sf::Keyboard::S:
					Keyboard[9] = 0;
					break;
				case sf::Keyboard::D:
					Keyboard[10] = 0;
					break;
				case sf::Keyboard::F:
					Keyboard[11] = 0;
					break;
				case sf::Keyboard::Z:
					Keyboard[12] = 0;
					break;
				case sf::Keyboard::X:
					Keyboard[13] = 0;
					break;
				case sf::Keyboard::C:
					Keyboard[14] = 0;
					break;
				case sf::Keyboard::V:
					Keyboard[15] = 0;
					break;
				case sf::Keyboard::M:
					std::cout << "Ingrese path a rom: ";
					std::cin >> RomPath;
					Reset();
				}
			default:
				break;
			}
			
		}
		Exec();

		if (RegisterBank.DelayTimer > 0) {
			RegisterBank.DelayTimer--;
		}
		if (RegisterBank.SoundTimer > 0) {
			Tone.play();
			RegisterBank.SoundTimer--;
		}
		else {
			Tone.stop();
		}

		window->clear();
		window->draw(frame);
		window->display();
	}
}

void Cpu::Exec() {
	if (RegisterBank.PC >= Memory.size()) {
		std::cerr << "Error PC sobrepaso a la size de memory" << std::endl;
		exit(0);
	}

	Opcode = (Memory[RegisterBank.PC] << 8);
	Opcode |= Memory[RegisterBank.PC + 1];

	switch (Opcode & 0xF000) {
	case 0x0000:
		Op0xxx();
		break;
	case 0x1000:
		Op1xxx();
		break;
	case 0x2000:
		Op2xxx();
		break;
	case 0x3000:
		Op3xxx();
		break;
	case 0x4000:
		Op4xxx();
		break;
	case 0x5000:
		Op5xxx();
		break;
	case 0x6000:
		Op6xxx();
		break;
	case 0x7000:
		Op7xxx();
		break;
	case 0x8000:
		Op8xxx();
		break;
	case 0x9000:
		Op9xxx();
		break;
	case 0xA000:
		OpAxxx();
		break;
	case 0xB000:
		OpBxxx();
		break;
	case 0xC000:
		OpCxxx();
		break;
	case 0xD000: {
		OpDxxx();
		break;
	}
	case 0xE000:
		OpExxx();
		break;
	case 0xF000:
		OpFxxx();
		break;
	default:
		std::cout << "Opcode Unrecognized " << std::hex << Opcode << std::endl;
		break;
	}

	if (RegisterBank.SoundTimer > 0) {
		RegisterBank.SoundTimer--;
	}
	if (RegisterBank.DelayTimer > 0) {
		RegisterBank.DelayTimer--;
	}
}

void Cpu::Op0xxx() {
	switch (Opcode & 0x000F) {
	case 0x0000:
#ifdef DEBUG
		std::cout << "CLS" << std::endl;
#endif
		for (int i = 0; i < 32 * 64; i++) {
			Framebuffer[i] = 0x00;
		}
		RegisterBank.PC += 2;
		break;
	case 0x000E:
#ifdef DEBUG
		std::cout << "RET" << std::endl;
#endif
		RegisterBank.PC = Stack[RegisterBank.SP];
		RegisterBank.SP--;
		RegisterBank.PC += 2;
		break;
	}
}

void Cpu::Op1xxx() {
#ifdef DEBUG
	std::cout << "JP " << std::setw(2) << std::setfill('0') << std::hex << (Opcode & 0x0FFF) << std::endl;
#endif
	RegisterBank.PC = (Opcode & 0x0FFF);
}

void Cpu::Op2xxx() {
#ifdef DEBUG
	std::cout << "Call " << std::setw(4) << std::setfill('0') << std::hex << (Opcode & 0x0FFF) << std::endl;
#endif
	RegisterBank.SP++;
	Stack[RegisterBank.SP] = RegisterBank.PC;
	RegisterBank.PC = (Opcode & 0x0FFF);
}

void Cpu::Op3xxx() {
#ifdef DEBUG
	std::cout << "SE" << std::endl;
#endif
	if (RegisterBank.V[((Opcode & 0x0F00) >> 8)] == (Opcode & 0x00FF)) {
		RegisterBank.PC += 2;
	}
	RegisterBank.PC += 2;
}

void Cpu::Op4xxx() {
#ifdef DEBUG
	std::cout << "SNE" << std::endl;
#endif
	if (RegisterBank.V[((Opcode & 0x0F00) >> 8)] != (Opcode & 0x00FF)) {
		RegisterBank.PC += 2;
	}
	RegisterBank.PC += 2;
}

void Cpu::Op5xxx() {
#ifdef DEBUG
	std::cout << "SE VX VY" << std::endl;
#endif
	if (RegisterBank.V[((Opcode & 0x0F00) >> 8)] == RegisterBank.V[((Opcode & 0x00F0) >> 4)]) {
		RegisterBank.PC += 2;
	}
	RegisterBank.PC += 2;
}

void Cpu::Op6xxx() {
#ifdef DEBUG
	std::cout << "LD V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], " << std::setw(2) << std::setfill('0') << std::hex << (Opcode & 0x00FF) << std::endl;
#endif
	RegisterBank.V[((Opcode & 0x0F00) >> 8)] = (Opcode & 0x00FF);
	RegisterBank.PC += 2;
}

void Cpu::Op7xxx() {
#ifdef DEBUG
	std::cout << "Add V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], " << (Opcode & 0x00FF) << std::endl;
#endif
	RegisterBank.V[((Opcode & 0x0F00) >> 8)] += (Opcode & 0x00FF);
	RegisterBank.PC += 2;
}

void Cpu::Op8xxx() {
	switch (Opcode & 0x000F) {
	case 0x0000:
#ifdef DEBUG
		std::cout << "LD V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], V[" << ((Opcode & 0x00F0) >> 4) << "]" << std::endl;
#endif
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] = RegisterBank.V[((Opcode & 0x00F0) >> 4)];
		RegisterBank.PC += 2;
		break;
	case 0x0001:
#ifdef DEBUG
		std::cout << "OR V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], V[" << ((Opcode & 0x00F0) >> 4) << "]" << std::endl;
#endif
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] |= RegisterBank.V[((Opcode & 0x00F0) >> 4)];
		RegisterBank.PC += 2;
		break;
	case 0x0002:
#ifdef DEBUG
		std::cout << "AND V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], V[" << ((Opcode & 0x00F0) >> 4) << "]" << std::endl;
#endif
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] &= RegisterBank.V[((Opcode & 0x00F0) >> 4)];
		RegisterBank.PC += 2;
		break;
	case 0x0003:
#ifdef DEBUG
		std::cout << "XOR V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], V[" << ((Opcode & 0x00F0) >> 4) << "]" << std::endl;
#endif
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] ^= RegisterBank.V[((Opcode & 0x00F0) >> 4)];
		RegisterBank.PC += 2;
		break;
	case 0x0004: {
#ifdef DEBUG
		std::cout << "ADD V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], V[" << ((Opcode & 0x00F0) >> 4) << "]" << std::endl;
#endif
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] += RegisterBank.V[((Opcode & 0x00F0) >> 4)];
		if (RegisterBank.V[((Opcode & 0x00F0) >> 4)] > (0xFF - RegisterBank.V[((Opcode & 0x0F00) >> 8)])) {
			RegisterBank.V[0x0F] = 1;
		}
		else {
			RegisterBank.V[0x0F] = 0;
		}

		RegisterBank.PC += 2;
	}
				 break;
	case 0x0005:
#ifdef DEBUG
		std::cout << "SUB V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], V[" << ((Opcode & 0x00F0) >> 4) << "]" << std::endl;
#endif
		if (RegisterBank.V[((Opcode & 0x0F00) >> 8)] > RegisterBank.V[((Opcode & 0x00F0) >> 4)]) {
			RegisterBank.V[0xF] = 1;
		}
		else {
			RegisterBank.V[0xF] = 0;
		}
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] -= RegisterBank.V[((Opcode & 0x00F0) >> 4)];
		RegisterBank.PC += 2;
		break;
	case 0x0006:
#ifdef DEBUG
		std::cout << "SHR V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		if (RegisterBank.V[((Opcode & 0x0F00) >> 8)] & 0x01 == 1) {
			RegisterBank.V[0xF] = 1;
		}
		else {
			RegisterBank.V[0xF] = 0;
		}
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] /= 2;
		RegisterBank.PC += 2;
		break;
	case 0x0007:
#ifdef DEBUG
		std::cout << "SUBN V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], V[" << ((Opcode & 0x00F0) >> 4) << "]" << std::endl;
#endif
		if (RegisterBank.V[((Opcode & 0x0F00) >> 8)] < RegisterBank.V[((Opcode & 0x00F0) >> 4)]) {
			RegisterBank.V[0xF] = 1;
		}
		else {
			RegisterBank.V[0x0F] = 0;
		}
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] -= RegisterBank.V[((Opcode & 0x00F0) >> 4)];
		RegisterBank.PC += 2;
		break;
	case 0x000E:
#ifdef DEBUG
		std::cout << "SHL V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		if (RegisterBank.V[((Opcode & 0x0F00) >> 8)] & 0x80 == 1) {
			RegisterBank.V[0xF] = 1;
		}
		else {
			RegisterBank.V[0xF] = 0;
		}
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] *= 2;
		RegisterBank.PC += 2;
		break;
	default:
		std::cout << "Opcode Unrecognized " << std::hex << Opcode << std::endl;
		break;
	}
}

void Cpu::Op9xxx() {
#ifdef DEBUG
	std::cout << "SNE VX VY" << std::endl;
#endif
	if (RegisterBank.V[((Opcode & 0x0F00) >> 8)] != RegisterBank.V[((Opcode & 0x00F0) >> 4)]) {
		RegisterBank.PC += 2;
	}
	RegisterBank.PC += 2;
}

void Cpu::OpAxxx() {
#ifdef DEBUG
	std::cout << "LD I, " << std::setw(2) << std::setfill('0') << std::hex << (Opcode & 0x0FFF) << std::endl;
#endif
	RegisterBank.I = (Opcode & 0x0FFF);
	RegisterBank.PC += 2;
}

void Cpu::OpBxxx() {
#ifdef DEBUG
	std::cout << "JMP V[0], " << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0FFF)) << std::endl;
#endif
	RegisterBank.PC = (RegisterBank.V[0] + (Opcode & 0x0FFF));
}

void Cpu::OpCxxx() {
#ifdef DEBUG
	std::cout << "RND V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], " << (Opcode & 0x00FF) << std::endl;
#endif
	RegisterBank.V[((Opcode & 0x0F00) >> 8)] = distribution(rd);
	RegisterBank.V[((Opcode & 0x0F00) >> 8)] &= (Opcode & 0x00FF);
	RegisterBank.PC += 2;
}

void Cpu::OpDxxx() {
	int Vx = RegisterBank.V[((Opcode & 0x0F00) >> 8)];
	int Vy = RegisterBank.V[((Opcode & 0x00F0) >> 4)];

#ifdef DEBUG
	std::cout << "DWR " << std::setw(4) << std::setfill('0') << std::hex << RegisterBank.I << ", " << Vx << ", " << Vy << ", " << (Opcode & 0x000F) << std::endl;
#endif

	RegisterBank.V[0xF] = 0;
	for (int y = 0; (y < (Opcode & 0x000F)); y++) {
		uint8_t pixel = Memory[RegisterBank.I + y];
		for (int x = 0; x < 8; x++) {
			if ((pixel & (0x80 >> x)) != 0) {
				if (Framebuffer[(x + Vx + ((y + Vy) * 64))] == 1) {
					RegisterBank.V[0x0F] = 1;
				}
				Framebuffer[(x + Vx + ((y + Vy) * 64))] ^= 1;
			}
		}
	}

	RegisterBank.PC += 2;
}

void Cpu::OpExxx() {
	switch ((Opcode & 0x000F)) {
	case 0x0001:
#ifdef DEBUG
		std::cout << "SKNP V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		if (Keyboard[RegisterBank.V[((Opcode & 0x0F00) >> 8)]] == 0) {
			RegisterBank.PC += 2;
		}
		RegisterBank.PC += 2;
		break;
	case 0x000E:
#ifdef DEBUG
		std::cout << "SKP V[" << std::setw(4) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		if (Keyboard[RegisterBank.V[((Opcode & 0x0F00) >> 8)]] == 1) {
			RegisterBank.PC += 2;
		}
		RegisterBank.PC += 2;
		break;
	}
}

void Cpu::OpFxxx() {
	switch (Opcode & 0x00FF) {
	case 0x0007:
#ifdef DEBUG
		std::cout << "LD V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], DT" << std::endl;
#endif
		RegisterBank.V[((Opcode & 0x0F00) >> 8)] = RegisterBank.DelayTimer;
		RegisterBank.PC += 2;
		break;
	case 0x000A:
#ifdef DEBUG
		std::cout << "LD V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], K" << std::endl;
#endif
		for (int i = 0; i < 16; i++) {
			if (Keyboard[i] == 1) {
				RegisterBank.V[((Opcode & 0x0F00) >> 8)] = i;
				RegisterBank.PC += 2;
				break;
			}
		}
		break;
	case 0x001E:
#ifdef DEBUG
		std::cout << "ADD I, V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], " << (Opcode & 0x00FF) << std::endl;
#endif
		RegisterBank.I += RegisterBank.V[((Opcode & 0x0F00) >> 8)];
		RegisterBank.PC += 2;
		break;
	case 0x0015:
#ifdef DEBUG
		std::cout << "LD DT, V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		RegisterBank.DelayTimer = RegisterBank.V[((Opcode & 0x0F00) >> 8)];
		RegisterBank.PC += 2;
		break;
	case 0x0018:
#ifdef DEBUG
		std::cout << "LD ST, V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		RegisterBank.SoundTimer = RegisterBank.V[((Opcode & 0x0F00) >> 8)];
		RegisterBank.PC += 2;
		break;
	case 0x0029:
#ifdef DEBUG
		std::cout << "LD F, V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		RegisterBank.I = RegisterBank.V[((Opcode & 0x0F00) >> 8)] * 5;
		RegisterBank.PC += 2;
		break;
	case 0x0033: {
#ifdef DEBUG
		std::cout << "LD B, V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		int tmp = RegisterBank.V[((Opcode & 0x0F00) >> 8)];
		Memory[RegisterBank.I] = tmp / 100;
		Memory[RegisterBank.I + 1] = ((tmp / 10) % 10);
		Memory[RegisterBank.I + 2] = ((tmp % 100) % 10);

		RegisterBank.PC += 2;
	}
				 break;

	case 0x0055:
#ifdef DEBUG
		std::cout << "LD [I], V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "]" << std::endl;
#endif
		for (int i = 0; i < ((Opcode & 0x0F00) >> 8); i++) {
			Memory[RegisterBank.I + i] = RegisterBank.V[i];
		}
		RegisterBank.PC += 2;
		break;
	case 0x0065:
#ifdef DEBUG
		std::cout << "LD V[" << std::setw(2) << std::setfill('0') << std::hex << ((Opcode & 0x0F00) >> 8) << "], [I]" << std::endl;
#endif
		for (int i = 0; i < ((Opcode & 0x0F00) >> 8); i++) {
			RegisterBank.V[i] = Memory[RegisterBank.I + i];
		}

		RegisterBank.I += (((Opcode & 0x0F00) >> 8) + 1);
		RegisterBank.PC += 2;
		break;
	default:
		std::cout << "Opcode Unrecognized " << std::hex << Opcode << std::endl;
		break;
	}
}