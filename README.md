# Lyric Cue FFGL — GitHub Builder v0.3

This version fixes the first GitHub Actions failure.

## What failed previously

The Resolume FFGL Visual Studio projects requested:

    Windows SDK 10.0.18362.0

The current GitHub Windows 2022 runner does not have that obsolete SDK
installed. The workflow now detects the Windows SDKs installed on the runner
and passes the newest installed SDK to MSBuild using:

    /p:WindowsTargetPlatformVersion=...

## How to use

1. Replace the files in your GitHub repository with this ZIP's contents.
2. Make sure the workflow is exactly:

    .github/workflows/build.yml

3. Go to GitHub -> Actions.
4. Select "Build Lyric Cue FFGL".
5. Click "Run workflow".
6. Wait for the build.
7. Download the artifact:

    LyricCue-Windows-x64

8. Extract it and copy LyricCue.dll to:

    Documents\Resolume\Extra Effects\

9. Restart Resolume Arena.

## Note

This build uses the official Resolume FFGL repository, whose README says the
master branch is intended for plugins compatible with Resolume 7.3.1 and up.
The official repository also lists FFGL 2.2 as its latest stable release.
