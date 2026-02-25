class RoomDevice {
  final String deviceId;
  final String roomId;
  final String? serverId;
  final String? clientId;
  final String name;
  final String deviceType;
  final bool isActive;

  const RoomDevice({
    required this.deviceId,
    required this.roomId,
    required this.serverId,
    required this.clientId,
    required this.name,
    required this.deviceType,
    required this.isActive,
  });

  factory RoomDevice.fromJson(Map<String, dynamic> json) {
    return RoomDevice(
      deviceId: json['device_id'] as String,
      roomId: json['room_id'] as String,
      serverId: json['server_id'] as String?,
      clientId: json['client_id'] as String?,
      name: json['name'] as String,
      deviceType: json['device_type'] as String,
      isActive: json['is_active'] as bool,
    );
  }
}
