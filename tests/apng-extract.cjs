const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert');
const [tool,dir]=process.argv.slice(2),source=fs.readFileSync('build/tween.apng'),out=path.join(dir,'extract.png'),input=path.join(dir,'malformed.apng');
const chunks=[];for(let p=8;p<source.length;){let n=source.readUInt32BE(p);chunks.push(Buffer.from(source.subarray(p,p+n+12)));p+=n+12;}
function crc(b){let c=0xffffffff;for(const x of b){c^=x;for(let k=0;k<8;k++)c=(c>>>1)^((c&1)?0xedb88320:0);}return(c^0xffffffff)>>>0;}
function mutate(type,offset,value){return chunks.map(c=>{c=Buffer.from(c);if(c.toString('ascii',4,8)===type){c.writeUInt32BE(value,8+offset);c.writeUInt32BE(crc(c.subarray(4,-4)),c.length-4);}return c;});}
const cases=[chunks.filter(c=>c.toString('ascii',4,8)!=='IHDR'),mutate('acTL',0,99),mutate('fcTL',0,99),mutate('fdAT',0,99),mutate('fcTL',12,1),chunks.slice(0,-1)];
for(const list of cases){fs.writeFileSync(input,Buffer.concat([source.subarray(0,8),...list]));const r=cp.spawnSync(tool,[input,'0',out],{encoding:'utf8'});assert.equal(r.status,2,r.stderr);assert(!fs.existsSync(out));}
for(const index of ['-1','0junk',''])assert.equal(cp.spawnSync(tool,['build/tween.apng',index,out]).status,2);
assert.equal(cp.spawnSync(tool,['build/tween.apng','0',out]).status,0);assert(fs.readFileSync(out).equals(fs.readFileSync('build/tween-0.png')));
console.log('APNG extraction rejects malformed structure and invalid frame indices');
