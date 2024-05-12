//=============================================================================================
// Mintaprogram: Zöld háromszög. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Vizhányó Miklós Ferenc
// Neptun : NVY1AG
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

using namespace std;

const char * const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers
	layout(location = 0) in vec3 vp;	// Varying input: vp = vertex position is expected in attrib array 0

	void main() {
		gl_Position = vec4(vp.x, vp.y, vp.z, 1);		// transform vp from modeling space to normalized device space
	}
)";


const char * const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers
	
	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(color, 1);	// computed color is the color of the primitive
	}
)";

GPUProgram gpuProgram;
unsigned int vao;

class Line{
    vec3 n;
    vec3 v;
    vec3 p0;
public:
    Line(vec3 p1, vec3 p2){
        v = p2-p1;
        n = normalize(vec3(-v.y, v.x, 0));
        p0=p1;
    }
    vec3 getN() { return n;}
    vec3 getV() { return v;}
    vec3 getP0() { return p0;}
    vec3 getIntersection(Line l1){
        if(dot(normalize(v), normalize(l1.n)) == 0)
            return vec3(0,0,0);
        double c = -dot(n,p0);
        double c1 = -dot(l1.n,l1.p0);
        double x = (n.y*c1-l1.n.y*c)/(n.x*l1.n.y-l1.n.x*n.y);
        double y = (l1.n.x*c-n.x*c1)/(n.x*l1.n.y-l1.n.x*n.y);
        return vec3(x,y,1);
    }
    bool isOn(vec3 p){
        if(fabs(dot(n,p)-dot(n,p0)) < 0.01f)
            return true;
        else
            return false;
    }
    vector<vec3> getSegment(){
        vector<vec3> v;
        vector<vec3> a;
        a.push_back(getIntersection( Line( vec3(-1,-1,1), vec3(-1,1,1) )));
        a.push_back(getIntersection( Line( vec3(1,1,1), vec3(1,-1,1) )));
        a.push_back(getIntersection( Line( vec3(-1,-1,1), vec3(1,-1,1) )));
        a.push_back(getIntersection( Line( vec3(-1,1,1), vec3(1,1,1) )));
        if(a[0].z!=0)
            v.push_back(a[0]);
        else
            v.push_back(a[2]);
        if(a[1].z!=0)
            v.push_back(a[1]);
        else
            v.push_back(a[3]);
        return v;
    }
    void move(vec3 p){
        p0=p;
    }
};

enum State {POINT, LINE, MOVE, INTERSECT};
State s = POINT;
vec3 * pickedPoint1 = nullptr;
vec3 * pickedPoint2 = nullptr;
Line * pickedLine1 = nullptr;
Line * pickedLine2 = nullptr;

void onKeyboard(unsigned char key, int pX, int pY) {
    switch(key) {
        case 'l':
            s = LINE;
            pickedPoint1 = nullptr;
            pickedPoint2 = nullptr;
            printf("Define lines\n");
            break;
        case 'm':
            s = MOVE;
            printf("Move\n");
            break;
        case 'i':
            s = INTERSECT;
            pickedLine1 = nullptr;
            pickedLine2 = nullptr;
            printf("Intersect\n");
            break;
        case 'p':
            s = POINT;
            printf("Define points\n");
            break;
    }
}

void onKeyboardUp(unsigned char key, int pX, int pY) {
}

void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME);
}

class Object {
    unsigned int vao, vbo;
    vector<vec3> vtx;
public:
    Object() {
        glGenVertexArrays(1, &vao); glBindVertexArray(vao);
        glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,NULL);
    }
    vector<vec3>& Vtx() { return vtx; }
    void setVtx(vector<vec3> vtx) {
        this->vtx = vtx;
    }
    void updateGPU() { // CPU -> GPU
        glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec3), &vtx[0], GL_DYNAMIC_DRAW);
    }
    void Draw(int type, vec3 color) {
        if (vtx.size() > 0) {
            glBindVertexArray(vao);
            gpuProgram.setUniform(color, "color");
            glDrawArrays(type, 0, vtx.size());
        }
    }
};

class PointCollection {
    Object points;
public:
    void addPoint(vec3 p) {
        points.Vtx().push_back(p);
        printf("Point %f, %f added\n", p.x, p.y);
    }
    void update() {
        points.updateGPU();
    }
    vec3 *pickPoint(vec3 pp) {
        for (auto& p : points.Vtx()) if (length(pp - p) < 0.05) return &p;
        return nullptr;
    }
    void Draw() {
        points.Draw(GL_POINTS, vec3(1, 0, 0));
    }
};

PointCollection * points;

class LineCollection{
    Object olines;
    vector<Line> vlines;
public:
    void add(Line l){
        olines.Vtx().push_back(l.getSegment()[0]);
        olines.Vtx().push_back(l.getSegment()[1]);
        vlines.push_back(l);
        printf("Line added\n"
               "\tImplicit: %f x + %f y + %f = 0\n"
               "\tParametric: r(t) = (%f, %f) + (%f, %f)t\n",
               l.getN().x, l.getN().y, dot(-l.getN(),l.getP0()),
               l.getP0().x, l.getP0().y, l.getV().x, l.getV().y);
    }
    Line* pickedLine(vec3 p){
        for (int i=0; i < vlines.size(); i++){
             if(vlines.at(i).isOn(p))
                 return &vlines[i];
        }
        return nullptr;
    }
    void update(){
        olines.Vtx().clear();
        for (int i=0; i < vlines.size(); i++){
            olines.Vtx().push_back(vlines[i].getSegment()[0]);
            olines.Vtx().push_back(vlines[i].getSegment()[1]);
        }
        olines.updateGPU();
    }
    void Draw() {
        olines.Draw(GL_LINES, vec3(0, 1, 1));
    }
};

LineCollection * lines;

vec3 PixelToNDC(int pX, int pY) {
    return vec3(2.0f * pX / windowWidth - 1, 1 - 2.0f * pY / windowHeight, 1);
}

void onMouse(int button, int state, int pX, int pY) {
    switch (s) {
            
        case POINT:
            if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
                points->addPoint(PixelToNDC(pX, pY));
                points->update(); glutPostRedisplay();
            }
            break;
            
        case LINE:
            if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
                if (pickedPoint1 == nullptr)
                    pickedPoint1 = points->pickPoint(PixelToNDC(pX, pY));
                else
                    pickedPoint2 = points->pickPoint(PixelToNDC(pX, pY));
                if(pickedPoint1 != nullptr && pickedPoint2 != nullptr){
                    lines->add(Line(vec3(pickedPoint1->x, pickedPoint1->y, 1), vec3(pickedPoint2->x, pickedPoint2->y, 1)));
                    lines->update(); glutPostRedisplay();
                    pickedPoint1 = nullptr;
                    pickedPoint2 = nullptr;
                }
            }
            break;
            
        case MOVE:
            if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && s == MOVE)
                pickedLine1 = lines->pickedLine(vec3(PixelToNDC(pX, pY)));
            if (button == GLUT_LEFT_BUTTON && state == GLUT_UP && s == MOVE)
                pickedPoint1 = nullptr;
            break;
        
        case INTERSECT:
            if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && s == INTERSECT){
                if (pickedLine1 == nullptr)
                    pickedLine1 = lines->pickedLine(vec3(PixelToNDC(pX, pY)));
                else
                    pickedLine2 = lines->pickedLine(vec3(PixelToNDC(pX, pY)));
                if(pickedLine1 != nullptr && pickedLine2 != nullptr){
                    vec3 p = pickedLine1->getIntersection(*pickedLine2);
                    if ( p.x<-1 || p.x>1 || p.y<-1 || p.y>1){
                        printf("out of viewport\n");
                    } else {
                        points->addPoint(p);
                        points->update(); glutPostRedisplay();
                    }
                    pickedLine1 = nullptr;
                    pickedLine2 = nullptr;
                }
            }
    }
}

void onMouseMotion(int pX, int pY) {
    if (pickedLine1 != nullptr) {
        pickedLine1->move(PixelToNDC(pX, pY));
        lines->update(); glutPostRedisplay();
    }
}

void onInitialization() {
    glViewport(0, 0, windowWidth, windowHeight);
    glLineWidth(3);
    glPointSize(10);
    points = new PointCollection;
    lines = new LineCollection;
    gpuProgram.create(vertexSource, fragmentSource, "fragmentColor");
}
void onDisplay() {
    glClearColor(0.3, 0.3, 0.3, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    lines->Draw();
    points->Draw();
    glutSwapBuffers();
}
