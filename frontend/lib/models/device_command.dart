import 'package:equatable/equatable.dart';

/// Command payload sent via REST to control a device.
///
/// Note: [toJson] was intentionally removed — the repository builds the
/// request body directly using the wire-level key names (`serverID`,
/// `devID`, etc.).  This class is kept as a typed value object.
class DeviceCommand extends Equatable {
  final String serverId;
  final String devId;
  final String comp;
  final int mod;
  final int stat;
  final int val;
  final String? reqId;

  const DeviceCommand({
    required this.serverId,
    required this.devId,
    required this.comp,
    required this.mod,
    required this.stat,
    required this.val,
    this.reqId,
  });

  @override
  List<Object?> get props => [serverId, devId, comp, mod, stat, val, reqId];
}
