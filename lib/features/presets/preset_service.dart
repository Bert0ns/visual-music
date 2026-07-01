import 'dart:io';
import 'package:logger/logger.dart';
import 'package:path/path.dart' as p;
import 'package:path_provider/path_provider.dart';
import 'package:sqflite_common_ffi/sqflite_ffi.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:visual_music/features/presets/preset_extractor.dart';

final presetServiceProvider = Provider<PresetService>((ref) {
  return PresetService();
});

class Preset {
  final int id;
  final String name;
  final String path;
  final bool isBanned;
  final bool isHearted;

  Preset({
    required this.id,
    required this.name,
    required this.path,
    required this.isBanned,
    required this.isHearted,
  });

  Preset copyWith({
    int? id,
    String? name,
    String? path,
    bool? isBanned,
    bool? isHearted,
  }) {
    return Preset(
      id: id ?? this.id,
      name: name ?? this.name,
      path: path ?? this.path,
      isBanned: isBanned ?? this.isBanned,
      isHearted: isHearted ?? this.isHearted,
    );
  }

  factory Preset.fromMap(Map<String, dynamic> map) {
    return Preset(
      id: map['id'],
      name: map['name'],
      path: map['path'],
      isBanned: map['is_banned'] == 1,
      isHearted: map['is_hearted'] == 1,
    );
  }
}

class PresetService {
  final _logger = Logger();
  Database? _db;

  Future<void> init() async {
    sqfliteFfiInit();
    databaseFactory = databaseFactoryFfi;

    final appDir = await getApplicationDocumentsDirectory();
    final dbPath = p.join(appDir.path, 'visual_music', 'presets.db');
    final presetsDir = p.join(appDir.path, 'visual_music', 'presets');

    await Directory(presetsDir).create(recursive: true);

    _db = await databaseFactory.openDatabase(
      dbPath,
      options: OpenDatabaseOptions(
        version: 1,
        onCreate: (db, version) async {
          await db.execute('''
            CREATE TABLE presets (
              id INTEGER PRIMARY KEY AUTOINCREMENT,
              name TEXT UNIQUE,
              path TEXT,
              is_banned INTEGER DEFAULT 0,
              is_hearted INTEGER DEFAULT 0
            )
          ''');
        },
      ),
    );

    final countResult = await _db!.rawQuery('SELECT COUNT(*) FROM presets');
    final count = countResult.first.values.first as int;
    if (count == 0) {
      final batchData = await PresetExtractor.extractAndIndexPresets(presetsDir);
      final batch = _db!.batch();
      for (final data in batchData) {
        batch.insert('presets', data);
      }
      await batch.commit(noResult: true);
    }
    
    // Extract textures
    _logger.i("Dart: Checking if textures are extracted...");
    final texturesDir = p.join(appDir.path, 'visual_music', 'textures');
    final textureFlagFile = File(p.join(texturesDir, '.extracted'));
    if (!await textureFlagFile.exists()) {
      _logger.i("Dart: Texture flag file not found. Extracting to $texturesDir...");
      await PresetExtractor.extractTextures(texturesDir);
      await textureFlagFile.writeAsString('done');
      _logger.i("Dart: Textures fully extracted!");
    } else {
      _logger.i("Dart: Textures already extracted.");
    }
    _logger.i("Dart: PresetService init complete.");
  }

  Future<Preset?> getRandomUnbannedPreset() async {
    if (_db == null) return null;
    
    final results = await _db!.rawQuery('''
      SELECT * FROM presets 
      WHERE is_banned = 0 
      ORDER BY RANDOM() 
      LIMIT 1
    ''');

    if (results.isNotEmpty) {
      return Preset.fromMap(results.first);
    }
    return null;
  }

  Future<void> toggleHeart(String presetPath, bool isHearted) async {
    if (_db == null) return;
    await _db!.update(
      'presets',
      {'is_hearted': isHearted ? 1 : 0},
      where: 'path = ?',
      whereArgs: [presetPath],
    );
  }

  Future<void> banPreset(String presetPath) async {
    if (_db == null) return;
    await _db!.update(
      'presets',
      {'is_banned': 1},
      where: 'path = ?',
      whereArgs: [presetPath],
    );
  }
}
