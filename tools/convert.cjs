// The page's converter from the command line (same convert.js and payload.js):
//   node tools/convert.cjs WOLF.g1a Wolf3D.g3a
const fs = require('fs');
const path = require('path');
const { convertG1a } = require('../convert.js');

const root = path.join(__dirname, '..');
const js = fs.readFileSync(path.join(root, 'payload.js'), 'utf8');
const payload = JSON.parse(js.slice(js.indexOf('{'), js.lastIndexOf('}') + 1));
const [input, output] = process.argv.slice(2);
if (!input || !output) {
  console.error('usage: node tools/convert.cjs WOLF.g1a Wolf3D.g3a');
  process.exit(1);
}
convertG1a(new Uint8Array(fs.readFileSync(input)), payload).then(
  (g3a) => { fs.writeFileSync(output, g3a); console.log(`${output}: ${g3a.length} bytes`); },
  (err) => { console.error(err.message); process.exit(1); });
