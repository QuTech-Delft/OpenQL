Release procedure
=================

Most of the release process is managed by GitHub Actions. If you follow the procedure below,
it will automatically test and build all wheels for all platforms,
and publish them to PyPI assuming the secrets are configured correctly.

 - Make a branch with a name starting with "release" based upon the commit that is to be released.
   For example, `release-0.0.1`, but the suffix doesn't matter.

 - Change the version in `include/ql/version.h` (this is the only functional place where the version is hardcoded)
   and change any other files where applicable (`CHANGELOG.md`, etc.), then commit and make a PR for it.

 - CI will now run not only the test workflow, but also the assets workflow.
   The assets workflow builds the wheels, but publishes them to the GitHub Actions build artifacts only.
   You can then test these yourself if you have reason to believe that
   something might be wrong with them that CI might not catch.
   To find them, go to Actions -> Assets workflow -> click the run for your branch -> Artifacts.

 - Always delete the artifacts of the assets runs when you're done with them or don't need them!
   OpenQL's binaries are quite big, so if you don't do this,
   GitHub will soon start rejecting new artifacts due to storage quota.

 - If the test or assets workflows fail, fix it before merging the PR (of course).

 - Once CI is green, merge the PR into `develop` if there are any changes.
   If no changes were needed, just delete the branch to clean up.

 - Create and push a new tag using the same version you put in `include/ql/version.h`.
   For example:

   ```
   git tag 0.0.1
   git push origin 0.0.1
   ```

 - Draft a new release through the GitHub interface.
   Set the "tag version", and the title to "Release `version`: `name`".
   And write release notes in the body, including at least:

    - a summary of what has changed, what is new, and what is incompatible with the previous version;
    - if there are incompatibilities, what the user can do for mitigation; and
    - what has been deprecated and may be removed in a later version.

   In principle, these things apply only to the public API.

 - CI will run the assets workflow again, now with the new version string baked into the wheels.
   When done, these wheels are automatically added to the GitHub release,
   and if the secrets/API keys for PyPI are correct, CI will publish them there.

 - Remove the temporary `release-*` branch.
   Users can find particular releases via the tag that GitHub automatically added when you drafted the release.
