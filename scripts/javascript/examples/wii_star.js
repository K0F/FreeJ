// freej example script by jaromil
// this simply draws i kind of star
// it also shows the usage of keyboard controller
// press 'q' to quit while running

include("param.js");

param = new Array();
param[0] = new Param(this, "px", 100, 150, 0, 1024, 500);
param[1] = new Param(this, "py", 100, 150, 0, 768,  500);
param[2] = new Param(this, "pz", 100, 150, 0, 1000, 500);


wii = new WiiController();

x = 100;
y = 100;
s = 30 / 0.383;

s2 = s;

PI = 3.141592654;
c = PI * 2;
o = -PI / 2;

wii.acceleration = function(ax,ay,az) {
    param[0].setValue(ax);
    param[1].setValue(ay);
    param[2].setValue(az);
    x = param[0].out_value;
    y = param[1].out_value;
    z = param[2].out_value;
}


function drawStar(lay, s_mul, s2_mul) {
    s = s_mul / 0.383;
    s2 = s / s2_mul;

    var cx;
    for(cx=0; cx<10; cx++) {
	k = cx/10;
	kn = k+0.1;
	
	x1 =   x+s * Math.cos(o + k*c);
	y1 =   y+s * Math.sin(o + k*c);
	x2 =   x+s2 * Math.cos(o + kn*c);
	y2 =   y+s2 * Math.sin(o + kn*c);
 	x1 = Math.floor(x1);
 	y1 = Math.floor(y1);
 	x2 = Math.floor(x2);
 	y2 = Math.floor(y2);

//	debug("drawing star line:" + x1 + "," + y1 + " " + x2 + "," + y2);
// 	lay.line( x.toPrecision()+s.toPrecision() * Math.cos(o + k*c),
// 		  y+s * Math.sin(o + k*c),
// 		  x+s2 * Math.cos(o + kn*c),
// 		  y+s2 * Math.sin(o + kn*c)  );
	lay.aaline(x1, y1, x2, y2);
    }
}


if(wii.connect())
    register_controller(wii);

wii.toggle_accel();


kbd = new KeyboardController();
kbd.pressed_esc = function() { quit(); }
kbd.released_q = function() { quit(); }
register_controller( kbd );

geo = new GeometryLayer();
geo.color(255,255,255,255);
geo.set_blit("alpha");
geo.set_blit_value(0.1);
geo.set_fps();
geo.start();
geo.activate(true);
add_layer(geo);

//drawStar(geo,30,1);
cc = 1;

srand();

bang = new TriggerController();

register_controller(bang);


bang.frame = function() {
    
    cc += 0.05;

    geo.color(rand()%255, rand()%255, rand()%255, 0xff);
    drawStar(geo, 30, Math.sin(cc));

    if(cc>4.0) {
	cc = 1;
	srand();
    }

}
