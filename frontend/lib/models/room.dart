import 'package:equatable/equatable.dart';

class Room extends Equatable {
  final String roomId;
  final String homeId;
  final String name;

  const Room({required this.roomId, required this.homeId, required this.name});

  factory Room.fromJson(Map<String, dynamic> json) {
    return Room(
      roomId: json['room_id'] as String,
      homeId: json['home_id'] as String,
      name: json['name'] as String,
    );
  }

  @override
  List<Object?> get props => [roomId, homeId, name];
}
