// Enclosure for Projekt GAS (LF07)
//
// Two separate enclosures:
//   1. Control unit (outside, next to the door frame):
//      membrane keypad, OLED, RFID-RC522, passive buzzer, Arduino Uno
//   2. Sensor unit (inside the room): HC-SR501 motion sensor
//
// Usage (OpenSCAD, free: https://openscad.org):
//   - choose a value for "part" below (or in the Customizer panel)
//   - F6 = render, then File > Export > Export as STL
//   - all *_back / *_front parts are already in print orientation
//
// All sizes in mm. Values in the section "Measured parts" come from
// data sheets - check them with a ruler before printing.

/* [Part selection] */
part = "assembly"; // [assembly, exploded, sensor_assembly, control_back, control_front, sensor_back, sensor_front]

/* [General] */
wall = 2.4;           // side wall thickness (6 lines with a 0.4 nozzle)
floor_t = 2.4;        // back plate thickness
lid_t = 2.4;          // front plate thickness
corner_r = 4;         // outer corner radius
clr = 0.3;            // gap between lid lip and walls
lip_t = 1.6;          // thickness of the lid lip
lip_h = 4;            // height of the lid lip
boss_d = 7;           // screw boss diameter
boss_inset = 6.5;     // screw boss centre, measured from the outer edge
pilot_d = 2.5;        // pilot hole for 3 mm screws
screw_d = 3.4;        // clearance hole for 3 mm screws
csk_d = 6.4;          // countersink diameter for the lid screws
wall_screw_d = 4.2;   // wall mounting screws (up to 4 mm)
wall_csk_d = 8;
guide_t = 1.2;        // thickness of the positioning guides
guide_h = 5;          // height of the positioning guides
fit = 0.25;           // play per side around modules
engrave = 0.6;        // depth of the engravings on the front

/* [Control unit] */
cu_w = 90;            // outer width
cu_depth = 40;        // inner depth (back plate to lid)

/* [Measured parts - check with a ruler!] */
keypad_w = 69.5;      // membrane keypad without ribbon
keypad_h = 77;
ribbon_slot_w = 23;   // must let the 8-pin plug through
ribbon_slot_h = 4;
rfid_w = 60;          // RC522 board
rfid_h = 40;
oled_w = 36;          // 1.3" SH1106 board
oled_h = 34;
oled_win_w = 32;      // window, active area is about 29.4 x 14.7
oled_win_h = 18;
buzzer_d = 12;        // buzzer body on the KY-006 style module
uno_standoff_h = 5;

/* [Sensor unit] */
su_w = 52;
su_h = 44;
su_depth = 30;        // inner depth, room for the plugs under the sensor
pir_w = 32.3;         // HC-SR501 board
pir_h = 24.3;
pir_hole_dist = 28;   // distance of the two mounting holes
pir_dome_d = 23;      // Fresnel lens diameter
pir_post_h = 3;       // gap between lid and sensor board

$fn = 48;

// ---------------------------------------------------------------------------
// Derived layout of the control unit
// Front view, origin = bottom-left outer corner, x right, y up, z to the front
// ---------------------------------------------------------------------------

UNO_W = 53.34;
UNO_L = 68.58;
// Uno mounting holes, board upright with the USB/power jacks at the bottom
UNO_HOLES = [[2.54, 15.24], [17.78, 66.04], [45.72, 66.04], [50.8, 13.97]];

lip_in = wall + clr + lip_t;          // outer edge -> inner side of the lip
cu_cx = cu_w / 2;
slot_y = lip_in + 0.8;                // ribbon slot directly below the keypad
kp_x = cu_cx - keypad_w / 2;
kp_y = slot_y + ribbon_slot_h;
rfid_cy = kp_y + keypad_h + 4 + rfid_h / 2;
oled_cy = rfid_cy + rfid_h / 2 + 3 + guide_t + oled_h / 2;
cu_h = ceil(oled_cy + oled_h / 2 + 3 + lip_in);
cu_th = floor_t + cu_depth;           // height of the back part
buzzer_x = (cu_cx + oled_w / 2 + fit + guide_t + cu_w - lip_in) / 2;

uno_x = cu_cx - UNO_W / 2;
uno_y = wall + 6.2 + 0.4;             // USB jack sticks out 6.2 mm
uno_z = floor_t + uno_standoff_h;

mount_y = [uno_y + UNO_L + 7, cu_h - 14];
cable_y = 145;
notch_w = 6;
notch_d = 5;

su_th = floor_t + su_depth;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

module rrect(w, h, r) {
    translate([r, r]) offset(r = r) square([w - 2 * r, h - 2 * r]);
}

// rounded rectangle shrunk by d on every side
module rrect_inset(w, h, r, d) {
    translate([d, d]) rrect(w - 2 * d, h - 2 * d, max(r - d, 0.5));
}

function boss_pos(w, h) = [
    [boss_inset, boss_inset], [w - boss_inset, boss_inset],
    [boss_inset, h - boss_inset], [w - boss_inset, h - boss_inset]
];

// countersunk hole, head at z = top
module csk_hole(d, dk, top) {
    cone_h = (dk - d) / 2;
    translate([0, 0, -1]) cylinder(d = d, h = top + 2);
    translate([0, 0, top - cone_h]) cylinder(d1 = d, d2 = dk, h = cone_h + 0.01);
    translate([0, 0, top]) cylinder(d = dk, h = 50);
}

// open box: back plate, walls and four screw bosses
module shell_back(w, h, th) {
    difference() {
        union() {
            difference() {
                linear_extrude(th) rrect(w, h, corner_r);
                translate([0, 0, floor_t])
                    linear_extrude(th) rrect_inset(w, h, corner_r, wall);
            }
            intersection() {
                linear_extrude(th) rrect(w, h, corner_r);
                for (p = boss_pos(w, h))
                    translate(p) cylinder(d = boss_d, h = th);
            }
        }
        for (p = boss_pos(w, h))
            translate([p[0], p[1], th - 12]) cylinder(d = pilot_d, h = 13);
    }
}

// lid: plate (z = 0 .. lid_t, front face on top) and lip hanging down
module shell_front(w, h) {
    difference() {
        union() {
            linear_extrude(lid_t) rrect(w, h, corner_r);
            translate([0, 0, -lip_h]) linear_extrude(lip_h) difference() {
                rrect_inset(w, h, corner_r, wall + clr);
                rrect_inset(w, h, corner_r, wall + clr + lip_t);
                for (p = boss_pos(w, h)) translate(p) circle(d = boss_d + 1);
            }
        }
        for (p = boss_pos(w, h))
            translate([p[0], p[1], -lip_h]) csk_hole(screw_d, csk_d, lip_h + lid_t);
    }
}

// cut a cable notch into the rim of a wall (x/y = centre of the notch)
module rim_notch(x, y, horizontal, th) {
    translate([x, y, th - notch_d + notch_w / 2]) {
        rotate(horizontal ? [90, 0, 0] : [0, 90, 0])
            cylinder(d = notch_w, h = 20, center = true);
        translate([0, 0, 25]) cube([horizontal ? notch_w : 20, horizontal ? 20 : notch_w, 50], center = true);
    }
}

// remove the lid lip where a rim notch is (x/y = centre of the notch)
module lip_cut(x, y, horizontal) {
    translate([x, y, -lip_h / 2 - 0.5])
        cube([horizontal ? notch_w + 2 : 20, horizontal ? 20 : notch_w + 2, lip_h + 1], center = true);
}

// four L-shaped corner guides around a w x h pocket, hanging below z = 0
module corner_guides(w, h, leg = 8) {
    pw = w + 2 * fit;
    ph = h + 2 * fit;
    translate([0, 0, -guide_h]) linear_extrude(guide_h) difference() {
        square([pw + 2 * guide_t, ph + 2 * guide_t], center = true);
        square([pw, ph], center = true);
        square([pw - 2 * leg, ph + 10], center = true);
        square([pw + 10, ph - 2 * leg], center = true);
    }
}

// line drawing of a rounded rectangle (for engravings)
module rrect_line(w, h, r, lw) {
    difference() {
        translate([-w / 2, -h / 2]) rrect(w, h, r);
        translate([-w / 2, -h / 2]) rrect_inset(w, h, r, lw);
    }
}

// L-shaped corner mark, legs pointing along +x and +y
module corner_mark(leg = 5, lw = 0.8) {
    square([leg, lw]);
    square([lw, leg]);
}

// ---------------------------------------------------------------------------
// Control unit
// ---------------------------------------------------------------------------

module control_back() {
    difference() {
        union() {
            shell_back(cu_w, cu_h, cu_th);
            // Uno standoffs
            for (p = UNO_HOLES)
                translate([uno_x + p[0], uno_y + p[1], 0])
                    cylinder(d = 6, h = uno_z);
        }
        // pilot holes for the Uno screws (deep enough for 3 x 8 mm)
        for (p = UNO_HOLES)
            translate([uno_x + p[0], uno_y + p[1], 0.8])
                cylinder(d = pilot_d, h = uno_z);
        // USB-B and power jack openings in the bottom wall
        translate([uno_x + 9.34 + 6 - 8, -1, uno_z - 1.5]) cube([16, wall + 2, 14]);
        translate([uno_x + 40.7 + 4.5 - 6, -1, uno_z - 1.5]) cube([12, wall + 2, 14]);
        // wall mounting holes, screw heads inside
        for (y = mount_y)
            translate([cu_cx, y, 0]) csk_hole(wall_screw_d, wall_csk_d, floor_t);
        // cable hole through the back (cable through the wall)
        translate([cu_cx, cable_y, -1]) cylinder(d = 8, h = floor_t + 2);
        // cable notches in both side walls (cable on the wall surface)
        rim_notch(wall / 2, cable_y, false, cu_th);
        rim_notch(cu_w - wall / 2, cable_y, false, cu_th);
    }
}

module control_front() {
    difference() {
        union() {
            shell_front(cu_w, cu_h);
            // RC522 pocket
            translate([cu_cx, rfid_cy, 0]) corner_guides(rfid_w, rfid_h);
            // OLED side guides - the board slides up/down until the
            // picture sits in the window, then a drop of hot glue
            for (s = [-1, 1])
                translate([cu_cx + s * (oled_w / 2 + fit + guide_t / 2) - guide_t / 2,
                           oled_cy - 10, -guide_h])
                    cube([guide_t, 20, guide_h]);
            // ring around the buzzer
            translate([buzzer_x, oled_cy, -4]) difference() {
                cylinder(d = buzzer_d + 2 * fit + 2 * guide_t, h = 4);
                translate([0, 0, -1]) cylinder(d = buzzer_d + 2 * fit, h = 6);
            }
        }
        // OLED window
        translate([cu_cx - oled_win_w / 2, oled_cy - oled_win_h / 2, -1])
            cube([oled_win_w, oled_win_h, lid_t + 2]);
        // keypad ribbon slot
        translate([cu_cx - ribbon_slot_w / 2, slot_y, -lip_h - 1])
            cube([ribbon_slot_w, ribbon_slot_h, lid_t + lip_h + 2]);
        // buzzer sound holes
        translate([buzzer_x, oled_cy, -1]) {
            cylinder(d = 2, h = lid_t + 2);
            for (a = [0 : 60 : 300])
                rotate(a) translate([3.5, 0, 0]) cylinder(d = 2, h = lid_t + 2);
        }
        // lip must not block the cable notches of the back part
        lip_cut(wall, cable_y, false);
        lip_cut(cu_w - wall, cable_y, false);
        // engravings on the front
        translate([0, 0, lid_t - engrave]) linear_extrude(engrave + 1) {
            // keypad corner marks
            translate([kp_x - 0.8, kp_y - 0.8]) corner_mark();
            translate([kp_x + keypad_w + 0.8, kp_y - 0.8]) mirror([1, 0]) corner_mark();
            translate([kp_x - 0.8, kp_y + keypad_h + 0.8]) mirror([0, 1]) corner_mark();
            translate([kp_x + keypad_w + 0.8, kp_y + keypad_h + 0.8]) rotate(180) corner_mark();
            // card symbol over the RFID reader
            translate([cu_cx, rfid_cy]) {
                rrect_line(50, 32, 4, 0.8);
                text("RFID", size = 7, font = "Liberation Sans:style=Bold",
                     halign = "center", valign = "center");
            }
            // project name left of the display (mirrors the buzzer holes)
            translate([cu_w - buzzer_x, oled_cy]) rotate(90)
                text("GAS", size = 6, font = "Liberation Sans:style=Bold",
                     halign = "center", valign = "center");
        }
    }
}

// ---------------------------------------------------------------------------
// Sensor unit
// ---------------------------------------------------------------------------

module sensor_back() {
    difference() {
        shell_back(su_w, su_h, su_th);
        for (x = [su_w / 2 - 12, su_w / 2 + 12])
            translate([x, su_h / 2, 0]) csk_hole(wall_screw_d, wall_csk_d, floor_t);
        translate([su_w / 2, su_h / 2, -1]) cylinder(d = 6, h = floor_t + 2);
        rim_notch(su_w / 2, wall / 2, true, su_th);
    }
}

module sensor_front() {
    difference() {
        union() {
            shell_front(su_w, su_h);
            for (s = [-1, 1])
                translate([su_w / 2 + s * pir_hole_dist / 2, su_h / 2, -pir_post_h])
                    cylinder(d = 4, h = pir_post_h);
        }
        // opening for the Fresnel lens
        translate([su_w / 2, su_h / 2, -1]) cylinder(d = pir_dome_d + 0.8, h = lid_t + 2, $fn = 96);
        // pilot holes for M2 screws
        for (s = [-1, 1])
            translate([su_w / 2 + s * pir_hole_dist / 2, su_h / 2, -pir_post_h - 1])
                cylinder(d = 1.6, h = pir_post_h + 2);
        lip_cut(su_w / 2, wall, true);
    }
}

// ---------------------------------------------------------------------------
// Dummy parts, only for the preview pictures
// ---------------------------------------------------------------------------

module uno_dummy() {
    color("teal") difference() {
        cube([UNO_W, UNO_L, 1.6]);
        for (p = UNO_HOLES) translate([p[0], p[1], -1]) cylinder(d = 3.2, h = 4);
    }
    color("silver") translate([9.34, -6.2, 1.6]) cube([12, 16, 10.9]);
    color("black") translate([40.7, -1.8, 1.6]) cube([9, 13.2, 10.9]);
    color("black") translate([0.8, 18, 1.6]) cube([2.5, 48, 8.5]);
    color("black") translate([50, 27, 1.6]) cube([2.5, 35, 8.5]);
    color("dimgray") translate([22, 30, 1.6]) cube([10, 35, 3.5]);
}

module keypad_dummy() {
    color("#222") cube([keypad_w, keypad_h, 0.8]);
    for (r = [0 : 3], c = [0 : 3])
        color(c == 3 ? "#d33" : (r == 0 && c < 3 ? "#eee" : "#eee"))
            translate([5 + c * 16, 5 + (3 - r) * 18, 0.8]) cube([12.5, 13, 0.4]);
}

module rfid_dummy() {
    color("#1e5bb8") translate([-rfid_w / 2, -rfid_h / 2, 0]) cube([rfid_w, rfid_h, 1.6]);
    color("#999") translate([rfid_w / 2 - 5, -10, -8]) cube([2.5, 20, 8]);
}

module oled_dummy() {
    color("#1e5bb8") translate([-oled_w / 2, -oled_h / 2, -1.6]) cube([oled_w, oled_h, 1.6]);
    color("#111") translate([-17.5, -11.5, 0]) cube([35, 23, 1.5]);
    color("#8cf") translate([-14.7, -7.35, 1.5]) cube([29.4, 14.7, 0.01]);
}

module buzzer_dummy() {
    color("#111") translate([0, 0, -8]) cylinder(d = buzzer_d, h = 8);
    color("#1e5bb8") translate([-7.5, -9.25, -9.6]) cube([15, 18.5, 1.6]);
}

module pir_dummy() {
    color("#2a7a2a") translate([-pir_w / 2, -pir_h / 2, -1.2]) cube([pir_w, pir_h, 1.2]);
    color("white") {
        translate([-12, -12, 0]) cube([24, 24, 2]);
        translate([0, 0, 2]) cylinder(d = pir_dome_d, h = 5);
        translate([0, 0, 7]) scale([1, 1, 0.5]) sphere(d = pir_dome_d);
    }
    color("#999") translate([-4, -pir_h / 2 + 1, -12]) cube([8, 2.5, 11]);
}

// ---------------------------------------------------------------------------
// Views
// ---------------------------------------------------------------------------

module control_assembly(lift = 0) {
    color("#e8e8e8") control_back();
    translate([uno_x, uno_y, uno_z]) uno_dummy();
    translate([0, 0, cu_th + lift]) {
        color("#f4b400") control_front();
        translate([kp_x, kp_y, lid_t]) keypad_dummy();
        translate([cu_cx, rfid_cy, -1.6]) rfid_dummy();
        translate([cu_cx, oled_cy, -1.5]) oled_dummy();
        translate([buzzer_x, oled_cy, 0]) buzzer_dummy();
    }
}

module sensor_assembly(lift = 0) {
    color("#e8e8e8") sensor_back();
    translate([0, 0, su_th + lift]) {
        color("#f4b400") sensor_front();
        translate([su_w / 2, su_h / 2, -pir_post_h]) pir_dummy();
    }
}

// print orientation of a lid: front face down on the bed
module flip_lid(w) {
    translate([w, 0, lid_t]) rotate([0, 180, 0]) children();
}

if (part == "assembly") {
    control_assembly();
    translate([cu_w + 30, 40, 0]) sensor_assembly();
} else if (part == "exploded") {
    control_assembly(lift = 45);
    translate([cu_w + 30, 40, 0]) sensor_assembly(lift = 35);
} else if (part == "sensor_assembly") {
    sensor_assembly(lift = 30);
} else if (part == "control_back") {
    control_back();
} else if (part == "control_front") {
    flip_lid(cu_w) control_front();
} else if (part == "sensor_back") {
    sensor_back();
} else if (part == "sensor_front") {
    flip_lid(su_w) sensor_front();
}

echo(str("Control unit: ", cu_w, " x ", cu_h, " x ", cu_th + lid_t, " mm"));
echo(str("Sensor unit:  ", su_w, " x ", su_h, " x ", su_th + lid_t, " mm"));
