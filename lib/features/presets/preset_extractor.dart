import 'dart:io';
import 'package:logger/logger.dart';
import 'package:flutter/foundation.dart';
import 'package:archive/archive_io.dart';
import 'package:flutter/services.dart' show rootBundle;
import 'package:path/path.dart' as p;

final _logger = Logger();

class PresetExtractor {
  static Future<List<Map<String, dynamic>>> extractAndIndexPresets(String outputDir) async {
    _logger.i("Extracting presets (this may take a minute)...");
    final zipData = await rootBundle.load('assets/presets.zip');
    final bytes = zipData.buffer.asUint8List();

    // Use compute to offload the heavy extraction to a background Isolate
    final batchData = await compute(_isolateExtract, {'bytes': bytes, 'outputDir': outputDir});
    
    _logger.i("Extraction and indexing complete.");
    return batchData;
  }

  static Future<void> extractTextures(String outputDir) async {
    _logger.i("Extracting textures...");
    await Directory(outputDir).create(recursive: true);
    final zipData = await rootBundle.load('assets/textures.zip');
    final bytes = zipData.buffer.asUint8List();
    await compute(_isolateExtractTextures, {'bytes': bytes, 'outputDir': outputDir});
    _logger.i("Textures extracted.");
  }
}

Future<List<Map<String, dynamic>>> _isolateExtract(Map<String, dynamic> args) {
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

void _isolateExtractTextures(Map<String, dynamic> args) {
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
