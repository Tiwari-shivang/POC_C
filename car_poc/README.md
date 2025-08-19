# Car PoC - Embedded C Automotive Features Simulation

A laptop-based embedded C application that simulates multiple in-vehicle features without hardware, following MISRA C principles and featuring both headless CSV-driven mode and interactive SDL2 UI.

## Features

- **Auto Braking**: Emergency braking when obstacles detected within threshold
- **Rain-Sensing Wipers**: Automatic wiper control based on rain intensity
- **Speed Governor**: Speed limit monitoring with traffic sign recognition
- **Auto Parking**: Simplified parking guidance system
- **Auto Climate Control**: Temperature control with discrete PI controller
- **Voice Assist**: Text-based voice command processing

## Build Requirements

### Dependencies
- GCC or Clang (C99 support)
- CMake >= 3.16
- SDL2 (for interactive mode, optional)
- Unity (included for unit tests)
- cppcheck (for static analysis)
- clang-tidy (for static analysis)
- clang-format (for code formatting)

### Windows Installation
```bash
# Install dependencies using vcpkg or similar
vcpkg install sdl2
```

### Linux Installation
```bash
sudo apt-get install gcc cmake libsdl2-dev cppcheck clang-tidy clang-format
```

## Building

### Headless Mode (Default)
```bash
cd car_poc
mkdir build && cd build
cmake -DHEADLESS=ON ..
make -j
```

### Interactive SDL2 Mode
```bash
cd car_poc
mkdir build && cd build
cmake -DHEADLESS=OFF ..
make -j
```

## Running

### Headless Mode
```bash
./car_poc --scenario ../cfg/scenario_default.csv
# Output logged to outputs.csv
```

### Interactive Mode
```bash
./car_poc
# Use keyboard controls:
# Arrow keys: adjust speed/distance
# R/F: adjust rain level
# 1/2/3/4: set speed limits (30/50/80/100 kph)
# P: toggle parking gap detection
# B: toggle driver brake override
# ESC: quit
```

### Command Line Options
- `--scenario <file>`: Specify input scenario CSV file
- `--help`: Show usage information

## Project Structure

```
car_poc/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── docs/                   # Documentation
│   ├── misra_policy.md     # MISRA compliance documentation
│   └── requirements.md     # Requirements specification
├── cfg/                    # Configuration files
│   ├── calib.h             # Calibration constants
│   └── scenario_default.csv # Default input scenario
├── inc/                    # Header files
│   ├── platform.h          # Platform abstraction
│   ├── hal.h               # Hardware abstraction layer
│   └── app_*.h             # Application module headers
├── src/                    # Source files
│   ├── main.c              # Main scheduler
│   ├── platform_*.c        # Platform implementations
│   ├── hal_*.c             # HAL implementations
│   └── app_*.c             # Feature modules (pure logic)
├── sim/                    # Simulation support
│   ├── scenario.h/.c       # CSV scenario parser
│   └── scenarios/          # Sample scenario files
├── tests/                  # Unit tests
│   ├── unity/              # Unity test framework
│   └── test_*.c            # Test files for each module
└── tools/                  # Development tools
    ├── run_static.sh       # Static analysis script
    └── format.sh           # Code formatting script
```

## Testing

### Unit Tests
```bash
cd build
ctest
```

### Static Analysis
```bash
./tools/run_static.sh
```

### Code Formatting
```bash
./tools/format.sh
```

## Architecture

### Embedded-Style Design
- **HAL Abstraction**: Clean separation between hardware and application logic
- **Deterministic Scheduler**: 10ms tick-based execution
- **Pure Application Modules**: No I/O dependencies in feature logic
- **MISRA-Minded**: Fixed-width types, no dynamic allocation, bounded execution

### Feature Modules
Each automotive feature is implemented as a separate module with:
- Initialization function (`*_init()`)
- Step function (`*_step()`) called every 10ms
- Pure logic with no side effects
- HAL-only I/O interface

### Build Modes
- **Headless**: Replays CSV scenarios, logs outputs for analysis
- **Interactive**: SDL2-based dashboard with keyboard controls

## Scenario Format

CSV files define time-series inputs:
```
ms,distance_mm,rain_pct,speed_kph,sign_event,gap_found,gap_width_mm,cabin_tc_x10,ambient_tc_x10,humid_pct,setpoint_x10,voice_cmd
0,2000,0,50,50,0,0,220,250,45,220,
100,1900,5,52,0,0,0,221,250,45,220,
...
```

## MISRA C Compliance

This project follows MISRA C guidelines including:
- Fixed-width integer types only
- No dynamic memory allocation
- No recursion
- Bounded loops and arrays
- Explicit type conversions
- Return value checking
- Static analysis integration

See [docs/misra_policy.md](docs/misra_policy.md) for detailed compliance information.

## Safety Features

- **Deterministic Timing**: 10ms scheduler tick
- **Sensor Validation**: Timestamp and staleness checking
- **Graceful Degradation**: Safe defaults on sensor failures
- **Override Protection**: Driver input always takes precedence
- **Bounded Execution**: All loops and counters have explicit limits

## Sample Scenarios

- `cfg/scenario_default.csv`: Basic feature demonstration
- `sim/scenarios/city_50kph.csv`: Urban driving with parking
- `sim/scenarios/highway_100kph.csv`: High-speed driving
- `sim/scenarios/rain_parking.csv`: Rainy weather parking scenario

## Development

### Adding New Features
1. Create header in `inc/app_newfeature.h`
2. Implement logic in `src/app_newfeature.c`
3. Add to scheduler in `src/main.c`
4. Create unit tests in `tests/test_newfeature.c`
5. Update CMakeLists.txt

### Coding Standards
- Follow MISRA C guidelines
- Use fixed-width types from `stdint.h`
- Initialize all variables
- Check all return values
- Keep functions small and focused
- Use static analysis tools

## License

This is a proof-of-concept implementation for automotive software development education and demonstration purposes.

## Contributing

Please ensure all contributions:
- Follow MISRA C guidelines
- Include unit tests
- Pass static analysis
- Update documentation as needed