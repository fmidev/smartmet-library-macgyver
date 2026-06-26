// ======================================================================
/*!
 * \brief Helpers for giving threads human-readable names
 *
 * The names show up in top/htop/gdb and greatly ease debugging. On Linux a
 * thread name is limited to 16 bytes including the terminating NUL, i.e. 15
 * visible characters; longer names are truncated. Keep names short and
 * informative (see the project naming scheme: ini-/upd-/ld-/sd- prefixes).
 */
// ======================================================================

#pragma once

#include <string>

namespace Fmi
{
/**
 * \brief Set a human-readable name on the calling thread.
 *
 * Truncated to 15 characters on Linux. If the given name is longer, a warning
 * is printed once per distinct name so that silent truncation does not hide
 * thread identities. No-op on platforms without pthread thread names.
 */
void set_thread_name(const std::string& name);

}  // namespace Fmi
