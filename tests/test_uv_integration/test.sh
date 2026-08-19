#!/bin/bash
set -e

# Regression test for the `uv` package manager integration
# (configure.ac UV detection + Makefile.am install-exec-hook).
#
# It does not run the full ./configure (LLVM detection, etc. make this
# slow and environment-dependent); instead it regenerates `configure`
# from configure.ac with `autoconf` alone and checks that the variables
# the install-exec-hook depends on are actually wired up. This is what
# catches issues like a missing AC_SUBST silently breaking `pip install`
# on Python >= 3.11 without uv installed.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

if ! command -v autoconf >/dev/null 2>&1 || ! command -v aclocal >/dev/null 2>&1; then
    echo "autoconf/aclocal not found, skipping"
    exit 77
fi

WORKDIR=$(mktemp -d)
trap 'rm -rf "$WORKDIR"' EXIT

# aclocal.m4 (macro definitions for AM_INIT_AUTOMAKE, AM_CONDITIONAL,
# AM_PATH_PYTHON, ...) is generated in the repo root by autogen.sh as
# part of the normal build; refresh it in place (cheap, gitignored
# build artifact) so autoconf has what it needs, then run autoconf
# alone (not autoreconf/automake, which would also try to regenerate
# every subdirectory's Makefile.in).
(cd "$REPO_ROOT" && aclocal -I m4 && autoconf -o "$WORKDIR/configure" configure.ac)

if ! grep -q '^UV=' "$WORKDIR/configure"; then
    echo "FAILED: UV is not substituted into configure (AC_PATH_PROG/AC_SUBST missing)"
    exit 1
fi

if ! grep -q '^PYTHON_INSTALL_FLAGS=\$PYTHON_INSTALL_FLAGS' "$WORKDIR/configure"; then
    echo "FAILED: PYTHON_INSTALL_FLAGS is not substituted into configure (AC_SUBST missing)"
    exit 1
fi

# Makefile.am's install-exec-hook must branch on uv availability and
# fall back to pip with the PEP 668 flags when uv is absent.
if ! grep -q 'test -n "\$(UV)"' "$REPO_ROOT/Makefile.am"; then
    echo "FAILED: install-exec-hook is missing the uv availability check"
    exit 1
fi

if ! grep -q '\$(UV) pip install' "$REPO_ROOT/Makefile.am"; then
    echo "FAILED: install-exec-hook is missing the 'uv pip install' branch"
    exit 1
fi

if ! grep -q '\$(PYTHON) -m pip install \$(PYTHON_INSTALL_FLAGS)' "$REPO_ROOT/Makefile.am"; then
    echo "FAILED: install-exec-hook is missing the pip fallback with PYTHON_INSTALL_FLAGS"
    exit 1
fi

echo "SUCCESS: uv integration is correctly wired in configure.ac / Makefile.am"
