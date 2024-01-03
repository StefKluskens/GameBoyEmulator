#include <iostream>
#include "imgui/SDL2-2.0.10/include/SDL.h"
#include "imgui/imgui.h"
#include "imgui/imgui_sdl.h"
#include "imgui/imgui_impl_sdl.h"
#include "../GameboyEmulator/EmulatorClean.h"
#include "../GameboyEmulator/GameBoy.h"

/*
	This is just a proof of concept, a quick example as to how you'd use the library
*/

#define INSTANCECOUNT 1

SDL_Window* wind{};
SDL_Renderer* rendr{};
SDL_Texture* textures[INSTANCECOUNT];
//gbee::Emulator emu{"Tetris(JUE)(V1.1)[!].gb", INSTANCECOUNT};
gbee::Emulator emu{"Dr.Mario.gb", INSTANCECOUNT};
//gbee::Emulator emu{"Batman-TheVideoGame(World).gb", INSTANCECOUNT};
//gbee::Emulator emu{"SuperMarioLand(JUE)(V1.1)[!].gb", INSTANCECOUNT};
//gbee::Emulator emu{"cpu_instrs.gb", INSTANCECOUNT};
//gbee::Emulator emu{"01-special.gb", INSTANCECOUNT};
//gbee::Emulator emu{"02-interrupts.gb", INSTANCECOUNT};
//gbee::Emulator emu{"03-op sp,hl.gb", INSTANCECOUNT};
//gbee::Emulator emu{"04-op r,imm.gb", INSTANCECOUNT};
//gbee::Emulator emu{"05-op rp.gb", INSTANCECOUNT};
//gbee::Emulator emu{"07-jr,jp,call,ret,rst.gb", INSTANCECOUNT};
//gbee::Emulator emu{"08-misc instrs.gb", INSTANCECOUNT};
//gbee::Emulator emu{"09-op r,r.gb", INSTANCECOUNT};
//gbee::Emulator emu{"11-op a,(hl).gb", INSTANCECOUNT};

void SetKeyState(const SDL_Event& event) {
	//Just ini 0
	if (event.type != SDL_KEYDOWN && event.type != SDL_KEYUP) return;

	gbee::Key key;
	switch (event.key.keysym.sym) {
			case SDLK_a:
				key = gbee::JOYPAD_A;
				break ;
			case SDLK_d:
				key = gbee::JOYPAD_B;
				break ;
			case SDLK_RETURN:
				key = gbee::JOYPAD_START;
				break ;
			case SDLK_SPACE:
				key = gbee::JOYPAD_SELECT;
				break ;
			case SDLK_RIGHT:
				key = gbee::JOYPAD_RIGHT;
				break ;
			case SDLK_LEFT:
				key = gbee::JOYPAD_LEFT;
				break ;
			case SDLK_UP:
				key = gbee::JOYPAD_UP;
				break ;
			case SDLK_DOWN:
				key = gbee::JOYPAD_DOWN;
				break ;
			default:
				return;
	}
	emu.SetKeyState( key, event.type == SDL_KEYDOWN, 0);
}

bool SDLEventPump() {
	SDL_Event e;
	while (SDL_PollEvent( &e )) {
		SetKeyState(e);
		ImGui_ImplSDL2_ProcessEvent( &e );
		if (e.type == SDL_QUIT) return false;
	}
	return true;
}

void CraftPixelBuffer( const uint8_t instance, uint16_t *buffer ) {
	std::bitset<(160 * 144) * 2> bitBuffer{ emu.GetFrameBuffer( instance ) };
	
#pragma loop(hint_parallel(20))
	for (int i{ 0 }; i < (160 * 144); ++i) {
		const uint8_t val{ uint8_t( (bitBuffer[i * 2] << 1) | uint8_t( bitBuffer[i * 2 + 1] ) ) };
		buffer[i] = (0xFFFF - (val * 0x5555));
	}

}

void Update() {

	const float fps{59.73f};
	bool autoSpeed[INSTANCECOUNT]{};
	int speedModifiers[INSTANCECOUNT];
	std::array<uint8_t, 160 * 144 * 4> viewportPixels{};
	viewportPixels.fill( 0xFF );

	while (SDLEventPump()) {

		ImGuiIO &io = ImGui::GetIO();
		io.DeltaTime = 1.0f / fps; //Runs a tick behind.. (Due to while loop condition)
		
		ImGui::NewFrame();

		//ImGui::ShowDemoWindow();
		uint16_t pixelBuffer[160*144]{};

		
		for (int i{ 0 }; i < INSTANCECOUNT; ++i) {
			const std::string name{ "Instance " };
			/*CraftPixelBuffer( i, pixelBuffer );
			SDL_UpdateTexture( textures[i],nullptr, (void*)pixelBuffer, 160*sizeof(uint16_t));*/

			if (emu.GetInstance(i).CanRender())
			{
				for (int j = 0; j < (144 * 160); j++)
				{
					Colour colour = emu.GetInstance(i).framebuffer[j];
					std::copy(colour.colours, colour.colours + 4, viewportPixels.begin() + (j * 4));
				}
				SDL_UpdateTexture(textures[i], nullptr, viewportPixels.data(), 160 * 4);
				speedModifiers[i] = emu.GetSpeed(i);

				emu.GetInstance(i).SetCanRender(false);
			}

			
			
			ImGui::Begin( (name + std::to_string( i )).c_str(), nullptr, ImGuiWindowFlags_NoResize );
				ImGui::SetWindowSize( {300, 230} );
				ImGui::Image( textures[i], { 160, 144 } );
				if(ImGui::Button( "Toggle Paused" ))
					emu.SetPauseState( !emu.GetPauseState( i ), i );
				
				if (ImGui::Checkbox( "AutoSpeed", autoSpeed+i))
					emu.SetAutoSpeed( autoSpeed[i], i );
				ImGui::SameLine(  );
				ImGui::SliderInt("Speed", &speedModifiers[i],1, 100);
				
			ImGui::End();
			emu.SetSpeed( speedModifiers[i], i );
		}
		//ImGui::ShowMetricsWindow();

		SDL_SetRenderDrawColor( rendr, 114, 144, 154, 255 );
		SDL_RenderClear( rendr );

		ImGui::Render();
		ImGuiSDL::Render( ImGui::GetDrawData() );

		SDL_RenderPresent( rendr );
	}

	ImGuiSDL::Deinitialize();

	SDL_DestroyRenderer( rendr );
	SDL_DestroyWindow( wind );
	for (int i{ 0 }; i < 1; ++i) {
		SDL_DestroyTexture( textures[i] );
	}
	
	ImGui::DestroyContext();
}

int main( int argc, char *argv[] ) {
    SDL_Init( SDL_INIT_EVERYTHING );
	emu.Start();

	wind = SDL_CreateWindow( "SDL2 ImGui Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_RESIZABLE );
	rendr = SDL_CreateRenderer( wind, -1, SDL_RENDERER_ACCELERATED );

	for (int i{ 0 }; i < INSTANCECOUNT; ++i) {
		//textures[i] = SDL_CreateTexture( rendr, SDL_PIXELFORMAT_RGBA4444, SDL_TEXTUREACCESS_STREAMING , 160, 144 );
		textures[i] = SDL_CreateTexture( rendr, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING , 160, 144 );

		SDL_SetRenderTarget( rendr, textures[i] );
		SDL_SetRenderDrawColor( rendr, 255, 0, 255, 255 );
		SDL_RenderClear( rendr );
		SDL_SetRenderTarget( rendr, nullptr );
	}
	
	ImGui::CreateContext();
	ImGuiSDL::Initialize( rendr, 800, 600 );
	
	Update();
	return 0;
}