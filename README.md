# ASCEND-GS

Ground station files for the ERAU ASCEND! team.

> **Note:** This repository should live under the [AscendERAU1](https://github.com/AscendERAU1) organization, not a personal account.

## Status

| Component | Status | Notes |
|---|---|---|
| Ground Station Mini | 🛠 In progress | Actively being edited |
| Arduino (New) | ✅ Current | Actively worked on |
| Python | ⚠️ Deprecated | Superseded by the Arduino implementation |


## Overview

Tracks the payload on the weather balloon with GPS coordinates and is able to accurately set azimuth and elevation to track. Receives data from the payload while in the air. Runs entirely off of [ASCEND_Ground_Station.ino](https://github.com/AscendERAU1/ASCEND-GS/blob/ascend-main/Arduino/New/Ino_codes/Arduino_Ground_Station/Arduino_Ground_Station.ino) and [AzimuthFull.h](https://github.com/AscendERAU1/ASCEND-GS/blob/ascend-main/Arduino/New/Ino_codes/Arduino_Ground_Station/AzimuthFull.h) respectively providing: code to move the ground station, code to calculate where the payload is in relation to the ground station and where to aim at.

## Repository Structure

```
.
├── ground-station-mini/   # Ground station mini — in progress
├── arduino/               # Current ground station firmware (active)
└── python/                # Deprecated — kept for reference only

```



## Getting Started

[ASCEND_Ground_Station_Instructions.md](https://github.com/AscendERAU1/ASCEND-GS/blob/ascend-main/Arduino/New/ASCEND_Ground_Station_Instructions.md)

## Contributing

This repo belongs to the ERAU ASCEND! team. Please open PRs against the appropriate component folder and note in your PR description whether it affects Arduino, Ground Station Mini, or another component.


