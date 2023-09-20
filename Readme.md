# DOSBlood
DOSBlood is a fork of the [Blood reconstruction by nukeykt](https://github.com/nukeykt/Blood-RE)

DOSBlood's goal is to provide quality-of-life features while retaining demo compatibility

### Downloads
Download can be found on [https://github.com/clipmove/DOSBlood/releases](https://github.com/clipmove/DOSBlood/releases)

### Installing
Backup your retail copy of `BLOOD.EXE` then replace with DOSBlood's `BLOOD.EXE`

### Features
* Add key icons to small HUD size
* Improved aim vector response by 33ms
* Vanilla mode option to restore original enemy bugs
* NBlood-style power-ups and level stats HUD display

### Fixes
* Fix game difficulty inverting on loading save
* Fix E4M8 cutscene playing on E4M1 start
* Initializes gSpriteHit on XSprite creation
* Fix Cerberus spinning on lava
* Fix infinite burning enemies bug
* Fix tiny Calebs using the wrong sprite
* Fix prone tesla Cultists infinitely firing
* Fix Beast state when leaving water sector
* Fix inventory items resetting between levels
* Allow cheat phrases to be said in multiplayer
* Fix choking hands to run at a constant framerate
* Fix pod enemy projectiles using walls as enemy index
* Fix demos desyncing if launched with -noaim argument
* Prevent whitespace only messages being sent in multiplayer
* Fix reverb state not resetting on level change/loading game
* Fix bloated butcher knife attack not hitting player while crouched
* Limit impulse damage when shooting enemies downward at point-blank
* Fix enemies always using tesla hit reaction after being hit by tesla projectile once
* Cheogh blasting/attacking can now hit prone players (only for well done and above difficulties)

### Notes
* You must already have an installed copy of Blood.
* Remember to keep a backup of your original executable!
* This is only for retail English version 1.21 (One Unit Whole Blood).
* For network, all other players must be using the same DOSBlood build else it will desync!

<details><summary><h3 dir="auto">Build instructions</h3></summary>
Watcom 10.6 and TASM 3.2 are required to build

1) Build helix32 and qtools (e.g. `cd helix32` and then `wmake`)
2) Build blood (e.g. `cd blood` and then `wmake`)</details>

Special thanks to nukeykt, NY00123, Hendricks266 and sirlemonhead