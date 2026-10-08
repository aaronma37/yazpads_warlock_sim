// Same splitmix64/xoshiro256** seed schedule as the Warlock shader.
// Independent source: no Warlock model/shader dependencies.
export const RNG_GLSL = `uvec2 add64(uvec2 a,uvec2 b){uint low=a.x+b.x;return uvec2(low,a.y+b.y+uint(low<a.x));}
uint highMultiply(uint a,uint b){
 uint a0=a&65535u,a1=a>>16u,b0=b&65535u,b1=b>>16u;
 uint w0=a0*b0,t=a1*b0+(w0>>16u),w1=(t&65535u)+a0*b1;
 return a1*b1+(t>>16u)+(w1>>16u);
}
uvec2 mul64(uvec2 a,uvec2 b){return uvec2(a.x*b.x,highMultiply(a.x,b.x)+a.x*b.y+a.y*b.x);}
uvec2 shr64(uvec2 a,uint n){if(n>=32u)return uvec2(a.y>>(n-32u),0u);return uvec2((a.x>>n)|(a.y<<(32u-n)),a.y>>n);}
uvec2 rot64(uvec2 a,uint n){uvec2 v=a;uint k=n;if(n>=32u){v=a.yx;k-=32u;}return (v<<k)|(v.yx>>(32u-k));}
void seedRandom(uint index){
 uvec2 x=add64(uvec2(seed,0u),uvec2(index,0u));
 if(all(equal(x,uvec2(0u))))x=uvec2(0xbeefcafeu,0x0000deadu);
 for(int i=0;i<4;i++){
  x=add64(x,uvec2(0x7f4a7c15u,0x9e3779b9u));
  uvec2 z=mul64(x^shr64(x,30u),uvec2(0x1ce4e5b9u,0xbf58476du));
  z=mul64(z^shr64(z,27u),uvec2(0x133111ebu,0x94d049bbu));rng[i]=z^shr64(z,31u);
 }
}
float random01(){
 uvec2 value=mul64(rot64(mul64(rng[1],uvec2(5u,0u)),7u),uvec2(9u,0u));
 uvec2 t=uvec2(rng[1].x<<17u,(rng[1].y<<17u)|(rng[1].x>>15u));
 rng[2]^=rng[0];rng[3]^=rng[1];rng[1]^=rng[2];rng[0]^=rng[3];rng[2]^=t;rng[3]=rot64(rng[3],45u);
 s.rngCalls++;return float(value.y>>8u)*(1.0/16777216.0);
}
`;
