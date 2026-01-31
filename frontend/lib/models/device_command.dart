class DeviceCommand {
  final String serverId;
  final String devId;
  final String comp;
  final int mod;
  final int stat;
  final int val;

  const DeviceCommand({
    required this.serverId,
    required this.devId,
    required this.comp,
    required this.mod,
    required this.stat,
    required this.val,
  });

  Map<String, dynamic> toJson() {
    return {
      'serverID': serverId,
      'devID': devId,
      'comp': comp,
      'mod': mod,
      'stat': stat,
      'val': val,
    };
  }
}
