# DOSBlood
DOSBlood is a fork of the [Blood reconstruction by nukeykt](https://github.com/nukeykt/Blood-RE).

DOSBlood's goal is to provide quality-of-life features while retaining demo compatibility.

### Features
* Fixes save difficulty bug
* Add key icons to small HUD size
* Improved mouse input response by 33ms
* Initializes gSpriteHit on XSprite creation
* Fixes inventory items resetting between levels

### Downloads
Download can be found on [https://github.com/clipmove/DOSBlood/releases](https://github.com/clipmove/DOSBlood/releases)

### Installing
Backup your retail copy of `BLOOD.EXE` then replace with DOSBlood's `BLOOD.EXE`.

### Notes
* You must already have an installed copy of Blood.
* Remember to keep a backup of your original executable!
* This is only for retail English version 1.21 (One Unit Whole Blood).
* For network, all other players must be using the same DOSBlood build else it will desync!

<details><summary><h3 dir="auto">Build instructions</h3></summary>
Watcom 10.6 and TASM 3.2 are required to build.

1) Build helix32 and qtools (e.g. `cd helix32` and then `wmake`)
2) Build blood (e.g. `cd blood` and then `wmake`)</details>

Special thanks to nukeykt, NY00123, Hendricks266 and sirlemonhead.
