import 'dart:io';
import 'package:logger/logger.dart';
import 'package:flutter/foundation.dart';
import 'package:archive/archive_io.dart';
import 'package:flutter/services.dart' show rootBundle;
import 'package:path/path.dart' as p;
import 'package:path_provider/path_provider.dart';
import 'package:sqflite_common_ffi/sqflite_ffi.dart';

final _logger = Logger();

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
  static final PresetService instance = PresetService._();
  PresetService._();

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
      await _extractAndIndexPresets(presetsDir);
    }
    
    // Extract textures
    _logger.i("Dart: Checking if textures are extracted...");
    final texturesDir = p.join(appDir.path, 'visual_music', 'textures');
    final textureFlagFile = File(p.join(texturesDir, '.extracted'));
    if (!await textureFlagFile.exists()) {
      _logger.i("Dart: Texture flag file not found. Extracting to $texturesDir...");
      await _extractTextures(texturesDir);
      await textureFlagFile.writeAsString('done');
      _logger.i("Dart: Textures fully extracted!");
    } else {
      _logger.i("Dart: Textures already extracted.");
    }
    _logger.i("Dart: PresetService init complete.");
  }

  Future<void> _extractTextures(String outputDir) async {
    _logger.i("Extracting textures...");
    await Directory(outputDir).create(recursive: true);
    final zipData = await rootBundle.load('assets/textures.zip');
    final bytes = zipData.buffer.asUint8List();
    await compute(isolateExtractTextures, {'bytes': bytes, 'outputDir': outputDir});
    _logger.i("Textures extracted.");
  }

  Future<void> _extractAndIndexPresets(String outputDir) async {
    _logger.i("Extracting presets (this may take a minute)...");
    final zipData = await rootBundle.load('assets/presets.zip');
    final bytes = zipData.buffer.asUint8List();

    // Use compute to offload the heavy extraction to a background Isolate
    final batchData = await compute(isolateExtract, {'bytes': bytes, 'outputDir': outputDir});

    final batch = _db!.batch();
    for (final data in batchData) {
      batch.insert('presets', data);
    }
    await batch.commit(noResult: true);
    _logger.i("Extraction and indexing complete.");
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

Future<List<Map<String, dynamic>>> isolateExtract(Map<String, dynamic> args) {
  final bytes = args['bytes'] as Uint8List;
  final outputDir = args['outputDir'] as String;

  final archive = ZipDecoder().decodeBytes(bytes);
  final List<Map<String, dynamic>> presets = [];

  for (final file in archive) {
    if (file.isFile && file.name.toLowerCase().endsWith('.milk')) {
      final filename = p.basename(file.name);
      final outputPath = p.join(outputDir, filename);

      final outputFile = File(outputPath);
      outputFile.createSync(recursive: true);
      outputFile.writeAsBytesSync(file.content as List<int>);

      presets.add({
        'name': p.basenameWithoutExtension(filename),
        'path': outputPath,
        'is_banned': 0,
        'is_hearted': 0,
      });
    }
  }

  return Future.value(presets);
}

void isolateExtractTextures(Map<String, dynamic> args) {
  final bytes = args['bytes'] as Uint8List;
  final outputDir = args['outputDir'] as String;

  final archive = ZipDecoder().decodeBytes(bytes);

  for (final file in archive) {
    if (file.isFile) {
      final filename = p.basename(file.name);
      final outputPath = p.join(outputDir, filename);

      final outputFile = File(outputPath);
      outputFile.createSync(recursive: true);
      outputFile.writeAsBytesSync(file.content as List<int>);
    }
  }
}
