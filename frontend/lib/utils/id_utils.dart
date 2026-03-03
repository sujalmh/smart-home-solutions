/// Shared ID normalization utilities.
///
/// The backend sometimes returns device IDs with the `RSW-` prefix and
/// sometimes without.  These helpers guarantee a consistent format across
/// repositories and screens.
library;

const String _prefix = 'RSW-';

/// Ensures [id] carries the `RSW-` prefix expected by the backend wire
/// protocol.  Returns [id] unchanged when it already has the prefix.
String normalizeId(String id) {
  if (id.startsWith(_prefix)) return id;
  return '$_prefix$id';
}

/// Returns the human-friendly portion of [id] by stripping the `RSW-`
/// prefix when present.
String displayId(String id) {
  if (id.startsWith(_prefix)) return id.substring(_prefix.length);
  return id;
}

/// Alias for [displayId] — returns the raw hardware ID without the
/// `RSW-` prefix.  Useful when building wire-level comparisons between
/// bound and discovered device lists.
String wireId(String id) => displayId(id);
