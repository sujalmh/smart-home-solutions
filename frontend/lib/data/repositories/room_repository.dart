import '../../models/home.dart';
import '../../models/room.dart';
import '../../models/room_device.dart';
import '../api_client.dart';

class RoomRepository {
  final ApiClient api;

  RoomRepository({required this.api});

  Future<List<Home>> listHomes() async {
    final response = await api.getList('/api/homes');
    return response
        .map((item) => Home.fromJson(item as Map<String, dynamic>))
        .toList();
  }

  Future<List<Room>> listRooms(String homeId) async {
    final response = await api.getList('/api/homes/$homeId/rooms');
    return response
        .map((item) => Room.fromJson(item as Map<String, dynamic>))
        .toList();
  }

  Future<List<RoomDevice>> listRoomDevices(String roomId) async {
    final response = await api.getList('/api/rooms/$roomId/devices');
    return response
        .map((item) => RoomDevice.fromJson(item as Map<String, dynamic>))
        .toList();
  }
}
