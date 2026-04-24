# BetterAngle Pro - Core Development Policy

## 1. No Local Builds
- All production releases MUST be handled by GitHub Actions.
- Do not provide or use local MSVC compilation scripts for releases unless explicitly requested.
- Maintain a clean "Absolute Truth" baseline in the remote repository.

## 2. No Draft Releases
- NEVER create draft releases on GitHub.
- All releases must be published as "Latest" or "Pre-release" immediately upon build completion.
- If a draft is automatically created by a pipeline error, it must be deleted or published immediately.

## 3. Version Synchronization
- Every release must have perfectly matching versions across:
    - `VERSION` file
    - `CMakeLists.txt`
    - `State.h`
    - `RELEASE_NOTES.md`

## 4. Input Ghosting Protection
- All FOV transitions must use the "Absolute Restoration" protocol (pre-lock snapshot + force-release).
- No kernel-level blocking that freezes the Windows Async Key Table without a corresponding restoration pulse.
