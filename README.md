## ClimbForge – Technical Release Notes

ClimbForge is an advanced Unreal Engine climbing movement system, inspired by the fluid traversal mechanics seen in The Legend of Zelda series (though not an exact replica). Designed for extensibility and as a technical showcase, this release highlights the following key Unreal Engine integrations and vector math features:

### Unreal Engine Features

- **Custom Movement Modes:**  
  Implements a custom climbing movement mode through UE’s extensible movement system (`MOVE_Custom`, `MOVE_Climbing`), including smooth transitions between walking, climbing, vaulting, and falling.

- **Component-Based Design:**  
  Utilizes custom components (`UClimbForgeMovementComponent`, `UMotionWarpingComponent`) for modularity and maintainability.

- **Blueprint & C++ Hybrid:**  
  Core climbing logic is written in C++ for optimal performance, exposed to Blueprints for designer-friendly tuning and animation montage integration.

- **Enhanced Input System:**  
  Full support for Unreal’s Enhanced Input system, with context-based input mappings for climbing, hopping, and movement actions.

- **Animation Integration:**  
  Custom `UCharacterAnimInstance` class synchronizes animation states (GroundSpeed, AirSpeed, ClimbVelocity) with real-time physics and climbing state.

- **AnimMontage Integration:**  
  Plays context-sensitive animation montages (e.g., climb, vault, hop) using Unreal’s animation system, with root motion support and event-driven state transitions.

- **Motion Warping:**  
  Employs Unreal’s Motion Warping component to dynamically adjust character position and orientation during complex traversal animations.

- **Trace & Collision Systems:**  
  Advanced use of line and capsule traces to detect climbable surfaces, ledges, and vault opportunities, configurable via Blueprint-exposed properties.

- **Trace Utilities:**  
  Implements capsule and line traces (`SweepMultiByChannel`, `LineTraceSingleByChannel`) for precise environment interaction, using Unreal’s collision and debug drawing systems.

- **Physics & Root Motion:**  
  Constrained root motion velocity and customized acceleration/deceleration for realistic climbing feel.

### Vector Math Highlights

- **Surface Normal and Direction Calculation:**  
  Extensively uses `FVector`, dot/cross products, and angle calculations to align the character with climbable surfaces and determine climb direction.

- **Climbable Angle Detection:**  
  Determines climbability via dot product and angle threshold (`theta = acos(a·b/|a||b|)`), ensuring only surfaces within a configurable angle can be climbed.

- **Directional Hopping:**  
  Determines hop direction and eligibility via vector dot products between input and local axes, supporting up, down, left, and right hops.

- **Ledge & Floor Detection:**  
  Uses vector-based logic to identify ledges and floors, enabling context-aware transitions between climbing, vaulting, and walking.

- **Input-Driven Movement Vectors:**  
  Translates 2D input vectors into 3D world-space movement using rotation matrices and axis projections.

- **Dynamic Climbing Velocity:**  
  Handles rotation and projection of velocity vectors to ensure movement is always aligned with the climbing surface, including “unrotating” velocity using quaternions (`UKismetMathLibrary::Quat_UnrotateVector`).

- **Trace Calculations:**  
  All traces (for climb, ledge, vault, etc.) are positionally offset and use vector math for direction, length, and surface normal evaluation.

## License and Usage

This project is licensed under the [Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International (CC BY-NC-ND 4.0)](https://creativecommons.org/licenses/by-nc-nd/4.0/).
[![License: CC BY-NC-ND 4.0](https://licensebuttons.net/l/by-nc-nd/4.0/88x31.png)](http://creativecommons.org/licenses/by-nc-nd/4.0/)

### What does this mean?
- **Personal/Non-Commercial Use:**  
  You are free to view, download, and share this project for personal and non-commercial purposes, as long as you provide appropriate credit to the author.

- **No Derivatives:**  
  You may not modify, remix, or create derivative works based on this project. Pull requests and contributions are not accepted.

- **Commercial Use:**  
  Commercial use of this project, in whole or in part, is not permitted without explicit written permission from the author. If you wish to use this project commercially, please contact [samarthshroff](mailto:samarthshroff@gmail.com) to discuss licensing and attribution requirements.

### Unreal Engine Content Notice
This project may include content (code, binaries, or assets) provided by Epic Games, Inc. (“Epic”) under the Unreal Engine End User License Agreement (EULA).  
Such content is subject to the [Unreal Engine EULA](https://www.unrealengine.com/eula) and may not be redistributed except as permitted by that agreement.

### Summary Table

| Use Case           | Allowed? | Notes                                                                 |
|--------------------|----------|-----------------------------------------------------------------------|
| Personal/Portfolio | ✅       | Credit required                                                      |
| Non-Commercial     | ✅       | Credit required                                                      |
| Commercial         | ❌       | Contact author for permission and credit requirements                 |
| Modification       | ❌       | No derivatives; do not submit pull requests or modified versions      |

For more details, see the [LICENSE](LICENSE) file in this repository.
