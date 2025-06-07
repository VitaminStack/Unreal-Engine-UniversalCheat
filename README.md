# Unreal-Engine-UniversalCheat

Based on https://github.com/Encryqed/Dumper-7
generate your own SDK for the Target Game and copy Paste the Dumped SDK in UE_UniversalCHeat/

An experimental cheat framework for Unreal Engine-based games, focusing on modularity and stability.

## Features
- Transparent overlays
- Pointer-safety (`PointerChecks`)
- Modular cheat management (add your own cheats easily)
- Easy-to-extend structure for new games or features

## Installation
1. Clone the project:  
   `git clone https://github.com/VitaminStack/Unreal-Engine-UniversalCheat.git`
2. Build with Visual Studio 2022 (64-bit Release).
3. Run **for testing and educational purposes only**!

## Important Notes
- This tool is not safe to use with any anti-cheat systems.
- Use at your own risk! The author assumes no liability.

## Adding Cheats
1. Extend `CheatList` in `Cheats.cpp`.
2. Implement your new cheat, for example:
   ```cpp
   CheatEntry myCheat = { "MyCheat", false, false, [] (bool state) { /* ... */ }, [] () { /* ... */ } };
   ```
3. Register your cheat in the list to have it appear in the overlay or toggle menu.

## Contributing
Pull requests and suggestions are welcome! For major changes, please open an issue first to discuss what you would like to change.

## License
This project is licensed under the MIT License. See the LICENSE file for details.

Rendering with Transparent Overlay in Scum:
![Beispielbild](https://github.com/VitaminStack/Unreal-Engine-UniversalCheat/blob/main/Screen.PNG)
