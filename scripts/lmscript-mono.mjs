#!/usr/bin/env node
// A narrow adapter for compiling _dsl_ blocks with the in-tree lmscript compiler
// when the full mono CLI dependencies have not been installed. It imports dsl.ts
// directly and never writes to the lmctl-mono checkout.
import {readFileSync, writeFileSync, mkdirSync} from 'node:fs';
import {basename, dirname, resolve, join} from 'node:path';
import {fileURLToPath, pathToFileURL} from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
const sourceRoot = process.env.LMSCRIPT_SOURCE ?? resolve(here, '../../../lmctl-mono/lmscript');
const dsl = await import(pathToFileURL(join(sourceRoot, 'dsl.ts')).href);
const args = process.argv.slice(2);

if (args.length === 1 && ['--version', '-v'].includes(args[0])) {
  console.log(`lmscript mono DSL v${dsl.DSL_VERSION} (${sourceRoot}/dsl.ts)`);
  process.exit(0);
}
if (args[0] === '--dsl-signatures') {
  if (args.length !== 2) { console.error('error: --dsl-signatures needs exactly one schema path'); process.exit(2); }
  const schemaPath = resolve(args[1]);
  const schema = dsl.parseDslSchema(readFileSync(schemaPath), schemaPath);
  console.log(dsl.canonicalSignatures(schema).join('\n'));
  process.exit(0);
}
if (args.length < 1 || args[0].startsWith('-')) {
  console.error('usage: lmscript-mono <source.lms> [output.dslb]');
  process.exit(2);
}

try {
  const sourcePath = resolve(args[0]);
  const source = readFileSync(sourcePath, 'utf8');
  const opener = /\b[A-Za-z_][A-Za-z0-9_]*\s*=\s*_dsl_\s+schema\s*=\s*("(?:\\.|[^"\\])*")\s*;/m.exec(source);
  if (!opener) throw new Error('could not find `_dsl_ schema="...";`');
  const bodyStart = opener.index + opener[0].length;
  const end = source.indexOf('end _dsl_', bodyStart);
  if (end < 0) throw new Error('could not find `end _dsl_`');
  const schemaName = JSON.parse(opener[1]);
  const schemaPath = resolve(dirname(sourcePath), schemaName);
  const schema = dsl.parseDslSchema(readFileSync(schemaPath), schemaPath);
  const compiled = dsl.compileDsl(source.slice(bodyStart, end).split(/\r?\n/), schema);
  const literalOutput = /savedsl\s*\(\s*[A-Za-z_][A-Za-z0-9_]*\s*,\s*("(?:\\.|[^"\\])*")\s*\)/m.exec(source);
  const outputPath = resolve(args[1] ?? (literalOutput ? JSON.parse(literalOutput[1]) : 'build/out.dslb'));
  mkdirSync(dirname(outputPath), {recursive: true});
  writeFileSync(outputPath, compiled.bytes);
  console.log(`wrote ${outputPath}`);
} catch (error) {
  console.error(`error:${basename(args[0])}: ${error instanceof Error ? error.message : String(error)}`);
  process.exit(2);
}
