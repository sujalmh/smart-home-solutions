class Home {
  final String homeId;
  final String name;

  const Home({required this.homeId, required this.name});

  factory Home.fromJson(Map<String, dynamic> json) {
    return Home(
      homeId: json['home_id'] as String,
      name: json['name'] as String,
    );
  }
}
