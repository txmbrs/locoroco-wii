export interface Color { r:number;g:number;b:number;a:number; }
export function colorNewFromRGBA(r:number,g:number,b:number,a=1):Color{return {r,g,b,a};}
export function colorFromRGBA(target:Color,r:number,g:number,b:number,a=1):void{Object.assign(target,{r,g,b,a});}
export function colorLerp(target:Color,start:Color,end:Color,t:number):void{
    for(const channel of ['r','g','b','a'] as const)target[channel]=start[channel]+(end[channel]-start[channel])*t;
}
