import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../../config/app_colors.dart';
import '../../config/app_decorations.dart';
import '../../models/room.dart';
import '../../models/room_device.dart';
import '../../state/providers.dart';

class RoomsDashboardScreen extends ConsumerStatefulWidget {
  const RoomsDashboardScreen({super.key});

  @override
  ConsumerState<RoomsDashboardScreen> createState() =>
      _RoomsDashboardScreenState();
}

class _RoomsDashboardScreenState extends ConsumerState<RoomsDashboardScreen> {
  String? _selectedHomeId;
  String? _selectedRoomId;

  @override
  Widget build(BuildContext context) {
    final homesAsync = ref.watch(homesProvider);

    return Scaffold(
      appBar: AppBar(title: const Text('Rooms Dashboard')),
      body: Container(
        decoration: BoxDecoration(
          gradient: AppDecorations.backgroundGradient(context.colors),
        ),
        child: homesAsync.when(
          data: (homes) {
            if (homes.isEmpty) {
              return const Center(child: Text('No homes available yet.'));
            }
            _selectedHomeId ??= homes.first.homeId;

            final roomsAsync = ref.watch(roomsProvider(_selectedHomeId!));
            return roomsAsync.when(
              data: (rooms) {
                if (rooms.isEmpty) {
                  return const Center(child: Text('No rooms configured.'));
                }
                _selectedRoomId ??= rooms.first.roomId;
                final roomIdSet = rooms.map((room) => room.roomId).toSet();
                if (!roomIdSet.contains(_selectedRoomId)) {
                  _selectedRoomId = rooms.first.roomId;
                }

                final devicesAsync = ref.watch(
                  roomDevicesProvider(_selectedRoomId!),
                );
                final wide = MediaQuery.of(context).size.width >= 900;

                if (wide) {
                  return Row(
                    children: [
                      _RoomRail(
                        rooms: rooms,
                        selectedRoomId: _selectedRoomId!,
                        onRoomSelected: (roomId) {
                          setState(() => _selectedRoomId = roomId);
                        },
                      ),
                      Expanded(
                        child: _DevicePanel(
                          roomName: _findRoomName(rooms, _selectedRoomId!),
                          devicesAsync: devicesAsync,
                        ),
                      ),
                    ],
                  );
                }

                return Column(
                  children: [
                    _RoomTabs(
                      rooms: rooms,
                      selectedRoomId: _selectedRoomId!,
                      onRoomSelected: (roomId) {
                        setState(() => _selectedRoomId = roomId);
                      },
                    ),
                    Expanded(
                      child: _DevicePanel(
                        roomName: _findRoomName(rooms, _selectedRoomId!),
                        devicesAsync: devicesAsync,
                      ),
                    ),
                  ],
                );
              },
              loading: () => const Center(child: CircularProgressIndicator()),
              error: (_, _) =>
                  const Center(child: Text('Unable to load rooms.')),
            );
          },
          loading: () => const Center(child: CircularProgressIndicator()),
          error: (_, _) => const Center(child: Text('Unable to load homes.')),
        ),
      ),
    );
  }

  String _findRoomName(List<Room> rooms, String roomId) {
    return rooms.firstWhere((room) => room.roomId == roomId).name;
  }
}

class _RoomRail extends StatelessWidget {
  final List<Room> rooms;
  final String selectedRoomId;
  final ValueChanged<String> onRoomSelected;

  const _RoomRail({
    required this.rooms,
    required this.selectedRoomId,
    required this.onRoomSelected,
  });

  @override
  Widget build(BuildContext context) {
    final selectedIndex = rooms.indexWhere(
      (room) => room.roomId == selectedRoomId,
    );

    return Container(
      width: 260,
      margin: const EdgeInsets.fromLTRB(16, 16, 12, 16),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: context.colors.cardBorder),
      ),
      child: NavigationRail(
        selectedIndex: selectedIndex < 0 ? 0 : selectedIndex,
        extended: true,
        minExtendedWidth: 240,
        labelType: NavigationRailLabelType.none,
        destinations: rooms
            .map(
              (room) => NavigationRailDestination(
                icon: const Icon(Icons.meeting_room_outlined),
                selectedIcon: const Icon(Icons.meeting_room),
                label: Text(room.name),
              ),
            )
            .toList(),
        onDestinationSelected: (index) => onRoomSelected(rooms[index].roomId),
      ),
    );
  }
}

class _RoomTabs extends StatelessWidget {
  final List<Room> rooms;
  final String selectedRoomId;
  final ValueChanged<String> onRoomSelected;

  const _RoomTabs({
    required this.rooms,
    required this.selectedRoomId,
    required this.onRoomSelected,
  });

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      height: 54,
      child: ListView.separated(
        padding: const EdgeInsets.fromLTRB(16, 12, 16, 8),
        scrollDirection: Axis.horizontal,
        itemCount: rooms.length,
        separatorBuilder: (_, _) => const SizedBox(width: 10),
        itemBuilder: (_, index) {
          final room = rooms[index];
          final selected = room.roomId == selectedRoomId;
          return ChoiceChip(
            label: Text(room.name),
            selected: selected,
            onSelected: (_) => onRoomSelected(room.roomId),
          );
        },
      ),
    );
  }
}

class _DevicePanel extends ConsumerWidget {
  final String roomName;
  final AsyncValue<List<RoomDevice>> devicesAsync;

  const _DevicePanel({required this.roomName, required this.devicesAsync});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(12, 16, 16, 16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            roomName,
            style: TextStyle(
              fontSize: 24,
              fontWeight: FontWeight.w700,
              color: context.colors.heading,
            ),
          ),
          const SizedBox(height: 6),
          Text(
            'Devices mapped to this room',
            style: TextStyle(color: context.colors.subtitle),
          ),
          const SizedBox(height: 14),
          Expanded(
            child: devicesAsync.when(
              data: (devices) {
                if (devices.isEmpty) {
                  return const Center(child: Text('No devices in this room.'));
                }
                return LayoutBuilder(
                  builder: (context, constraints) {
                    final crossAxisCount = constraints.maxWidth >= 860 ? 3 : 1;
                    return GridView.builder(
                      gridDelegate: SliverGridDelegateWithFixedCrossAxisCount(
                        crossAxisCount: crossAxisCount,
                        crossAxisSpacing: 12,
                        mainAxisSpacing: 12,
                        childAspectRatio: 1.35,
                      ),
                      itemCount: devices.length,
                      itemBuilder: (_, index) =>
                          _RoomDeviceCard(device: devices[index]),
                    );
                  },
                );
              },
              loading: () => const Center(child: CircularProgressIndicator()),
              error: (_, _) =>
                  const Center(child: Text('Unable to load devices.')),
            ),
          ),
        ],
      ),
    );
  }
}

class _RoomDeviceCard extends StatelessWidget {
  final RoomDevice device;

  const _RoomDeviceCard({required this.device});

  @override
  Widget build(BuildContext context) {
    final c = context.colors;
    final onlineColor = device.isActive ? c.online : c.neutral;

    return Container(
      padding: const EdgeInsets.all(16),
      decoration: AppDecorations.card(c),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Container(
                padding: const EdgeInsets.symmetric(
                  horizontal: 10,
                  vertical: 5,
                ),
                decoration: AppDecorations.statusChip(onlineColor),
                child: Text(
                  device.isActive ? 'Active' : 'Inactive',
                  style: TextStyle(
                    color: onlineColor,
                    fontWeight: FontWeight.w600,
                  ),
                ),
              ),
              const Spacer(),
              const Icon(Icons.memory_outlined),
            ],
          ),
          const SizedBox(height: 10),
          Text(
            device.name,
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
            style: const TextStyle(fontSize: 17, fontWeight: FontWeight.w700),
          ),
          const SizedBox(height: 4),
          Text(
            device.clientId ?? '-',
            style: TextStyle(color: c.subtitle),
          ),
          const Spacer(),
          Container(
            width: double.infinity,
            padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
            decoration: BoxDecoration(
              color: c.surfaceAlt,
              borderRadius: BorderRadius.circular(kChipRadius),
            ),
            child: Text(
              device.deviceType,
              style: const TextStyle(fontWeight: FontWeight.w600),
            ),
          ),
        ],
      ),
    );
  }
}
