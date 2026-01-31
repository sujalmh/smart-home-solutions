import 'dart:async';

import 'package:socket_io_client/socket_io_client.dart' as io;

class SocketClient {
  final String url;
  final String path;
  io.Socket? _socket;
  final _responseController =
      StreamController<Map<String, dynamic>>.broadcast();
  final _statusController = StreamController<Map<String, dynamic>>.broadcast();

  SocketClient({required this.url, required this.path});

  Stream<Map<String, dynamic>> get responses => _responseController.stream;
  Stream<Map<String, dynamic>> get status => _statusController.stream;

  void connect({String? token}) {
    final options = io.OptionBuilder()
        .setTransports(['websocket'])
        .setPath(path)
        .disableAutoConnect()
        .build();

    if (token != null && token.isNotEmpty) {
      options['auth'] = {'token': token};
    }

    _socket = io.io(url, options);
    _socket?.on('response', (data) => _responseController.add(_asMap(data)));
    _socket?.on('staresult', (data) => _statusController.add(_asMap(data)));
    _socket?.connect();
  }

  void disconnect() {
    _socket?.disconnect();
    _socket?.dispose();
    _socket = null;
  }

  void emit(String event, Map<String, dynamic> data) {
    _socket?.emit(event, data);
  }

  void dispose() {
    disconnect();
    _responseController.close();
    _statusController.close();
  }

  Map<String, dynamic> _asMap(dynamic data) {
    if (data is Map<String, dynamic>) {
      return data;
    }
    if (data is Map) {
      return data.map((key, value) => MapEntry(key.toString(), value));
    }
    return {};
  }
}
