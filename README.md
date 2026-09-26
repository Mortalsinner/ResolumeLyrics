# Lyric Cue — GitHub Actions Builder

This package lets GitHub compile the Resolume FFGL plugin on a Windows
GitHub Actions runner. You do NOT need Visual Studio installed locally.

## How to use

1. Create a new GitHub repository.
2. Upload the contents of this ZIP to the repository.
3. Open the repository on GitHub.
4. Go to Actions.
5. Select "Build Lyric Cue FFGL".
6. Click "Run workflow".
7. Wait for the build to finish.
8. Open the completed workflow run.
9. Download the artifact named:

   LyricCue-Windows-x64

The downloaded artifact contains:

   LyricCue.dll

## Install into Resolume

Close Resolume Arena, then copy:

   LyricCue.dll

to:

   Documents\Resolume\Extra Effects\

Restart Arena.

## Important

This workflow uses a GitHub-hosted Windows runner and Microsoft's MSVC
toolchain supplied by the runner. Visual Studio does not need to be installed
on your own PC.

The workflow checks out the current Resolume FFGL repository and builds its
Windows x64 solution. It replaces the Gradients example implementation with
the Lyric Cue implementation.

If the official FFGL repository changes its project structure/API, the GitHub
Actions build may fail. In that case, send the Actions build log and the
project can be updated.
