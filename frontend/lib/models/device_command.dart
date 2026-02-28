class DeviceCommand {
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

  Map<String, dynamic> toJson() {
    return {
      'serverID': serverId,
      'devID': devId,
      'comp': comp,
      'mod': mod,
      'stat': stat,
      'val': val,
      if (reqId != null) 'reqId': reqId,
    };
  }
}
