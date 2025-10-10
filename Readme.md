# DOSBlood
DOSBlood is a fork of the [Blood reconstruction by nukeykt](https://github.com/nukeykt/Blood-RE)

DOSBlood's provides quality-of-life features while retaining Blood 1.21 demo and network compatibility

### Downloads
Download can be found on [https://github.com/clipmove/DOSBlood/releases](https://github.com/clipmove/DOSBlood/releases)

### Installing
Backup your retail copy of `BLOOD.EXE` then replace with DOSBlood's `BLOOD.EXE`

### Features
* Auto crouch
* Field of view slider
* Add last weapon button
* Autosave on level start
* Center horizon line option
* Show map title on level start
* Add key icons to small HUD size
* BloodGDX style difficulty options
* Add autoaim toggle in options menu
* Texture panning interpolation support
* New 'KRAVITZ' cheat code for fly mode
* Improved aim vector response by 33ms
* Increased max sector interpolations by 512
* Vanilla mode option to restore original enemy bugs
* Supports multiplayer with Blood version 1.21 clients
* NBlood-style power-ups and level stats HUD display
* New 'LIMITS' cheat code to display wall/sprites usage
* Restore broken blinking state for health point counter
* High precision weapon sway calculation and interpolation
* Don't restart midi track when loading save for current level
* Incorporates mouse fixes from bMouse and sMouse natively
* Use Pentium II/PRO (if found) optimizations for a.asm functions (12% speed improvement)

### Fixes
* Fix game difficulty inverting on loading save
* Fix E4M8 cutscene playing on E4M1 start
* Initializes gSpriteHit on XSprite creation
* Clear interpolation queue on new level
* Fix life leech shadow
* Fix Cerberus spinning on lava
* Ignore floor pal zero for sectors
* Fix infinite burning enemies bug
* Fix tiny Calebs using the wrong sprite
* Fix 2D map view not scaling to resolution
* Fix prone tesla Cultists infinitely firing
* Fix self collisions for lifeleech projectiles
* Fix Beast state when leaving water sector
* Fix enemy health resetting on loading save
* Update delirium tilt at a constant framerate
* Fix cultists screaming getting cut on death
* Fix inventory items resetting between levels
* Fix shotgun akimbo animation glitch on loop
* Fixed underwater issue with hitscan weapons
* Allow cheat phrases to be said in multiplayer
* Reset view bob/view sway inertia on level start
* Fix weapon state not resetting on weapon change
* Fix fall scream triggering after player has died
* Fix choking hands to run at a constant framerate
* Fix secrets for maps with glitched secret triggers
* Fix projectiles glitching when autoaim is disabled
* Fix pod enemy projectiles using walls as enemy index
* Fix spray can glitching when alt-firing and entering water
* Fix demos desyncing if launched with -noaim argument
* Replaced sector based damage logic for player explosions
* Fix shotgun getting stuck in single gun state while akimbo
* Prevent whitespace only messages being sent in multiplayer
* Fix reverb state not resetting on level change/loading game
* Check if voxel exists before attempting to draw tile as voxel
* Fix demo recording desyncing when pausing/opening menu
* Fix bloated butcher knife attack not hitting player while crouched
* Don't reset inertia when traveling through room-over-room sectors
* Force TNT/spray cans to explode if directly landed on enemy's head
* Limit impulse damage when shooting enemies downward at point-blank
* Correct ear velocity when transitioning through room-over-room sectors
* Fix shadows and flare glow effects rendering on room-over-room surfaces
* Fix next/previous weapon buttons not skipping TNT/spray can while underwater
* Fix enemies always using tesla hit reaction after being hit by tesla projectile once
* Cheogh blasting/attacking can now hit prone players (only for well done and above difficulties)
* Fix interpolated sprite interpolation resetting when transitioning through room-over-room sectors

### Notes
* You must already have an installed copy of Blood.
* Remember to keep a backup of your original executable!
* This is only for retail English version 1.21 (One Unit Whole Blood)

### Build instructions
Watcom 10.6, MASM 5.10 and TASM 3.1 are required to build

1) Build helix32, qtools and build (i.e. `cd helix32` and then `wmake`)
2) Build blood (i.e. `cd blood` and then `wmake`)

Note: The Build Engine (by Ken Silverman) code has been effortlessly recreated by the [gamesrc-ver-recreation project](https://bitbucket.org/gamesrc-ver-recreation/build/src/master/), and compiles close enough that DOSBlood can playback 11 hours of demo files without desyncronizing. If you prefer to use the original objects file, run `clean.bat` and then `origlib.bat` in the build directory

Special thanks to nukeykt, NY00123, Hendricks266, sirlemonhead and Maxi Clouds.