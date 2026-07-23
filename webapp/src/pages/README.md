# Pages

Hash routing is implemented in `src/App.tsx` for all planned application screens.
Unknown routes return to login.

The current route implementations are still centralized in `App.tsx`; feature-owned
page components have not yet been split into this directory. New route-level
composition should move here as the real set, macro, procedure, execution, and
settings workflows are implemented.

A route existing does not imply its backend workflow is complete. See the feature
README and `docs/IMPLEMENTATION_STATUS.md` before presenting a screen as functional.
