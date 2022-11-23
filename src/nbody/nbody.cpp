#if defined(__APPLE__) || defined(MACOSX)
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <cassert>
#include <cmath>
#include "bodysystemcpu.h"
#include "render_particles.h"
#include "helper_timer.h"
#include "paramgl.h"

// view params
int ox = 0, oy = 0;
int buttonState = 0;
float camera_trans[] = {0, -2, -150};
float camera_rot[] = {0, 0, 0};
float camera_trans_lag[] = {0, -2, -150};
float camera_rot_lag[] = {0, 0, 0};
const float inertia = 0.1f;

ParticleRenderer::DisplayMode displayMode =
    ParticleRenderer::PARTICLE_SPRITES_COLOR;

bool benchmark = false;
bool compareToCPU = false;
bool QATest = false;
int blockSize = 256;
bool useHostMem = false;
bool useP2P = true;  // this is always optimal to use P2P path when available
bool fp64 = false;
bool useCpu = false;
int numDevsRequested = 1;
bool displayEnabled = true;
bool bPause = false;
bool bFullscreen = false;
bool bDispInteractions = false;
bool bSupportDouble = false;
int flopsPerInteraction = 20;

char deviceName[100];

enum { M_VIEW = 0, M_MOVE };

int numBodies = 16384;

std::string tipsyFile = "";

int numIterations = 0;  // run until exit

void computePerfStats(double &interactionsPerSecond, double &gflops,
                      float milliseconds, int iterations) {
  // double precision uses intrinsic operation followed by refinement,
  // resulting in higher operation count per interaction.
  // (Note Astrophysicists use 38 flops per interaction no matter what,
  // based on "historical precedent", but they are using FLOP/s as a
  // measure of "science throughput". We are using it as a measure of
  // hardware throughput.  They should really use interactions/s...
  // const int flopsPerInteraction = fp64 ? 30 : 20;
  interactionsPerSecond = (float)numBodies * (float)numBodies;
  interactionsPerSecond *= 1e-9 * iterations * 1000 / milliseconds;
  gflops = interactionsPerSecond * (float)flopsPerInteraction;
}

////////////////////////////////////////
// Demo Parameters
////////////////////////////////////////
struct NBodyParams {
  float m_timestep;
  float m_clusterScale;
  float m_velocityScale;
  float m_softening;
  float m_damping;
  float m_pointSize;
  float m_x, m_y, m_z;

  void print() {
    printf("{ %f, %f, %f, %f, %f, %f, %f, %f, %f },\n", m_timestep,
           m_clusterScale, m_velocityScale, m_softening, m_damping, m_pointSize,
           m_x, m_y, m_z);
  }
};

NBodyParams demoParams[] = {
    {0.016f, 1.54f, 8.0f, 0.1f, 1.0f, 1.0f, 0, -2, -100},
    {0.016f, 0.68f, 20.0f, 0.1f, 1.0f, 0.8f, 0, -2, -30},
    {0.0006f, 0.16f, 1000.0f, 1.0f, 1.0f, 0.07f, 0, 0, -1.5f},
    {0.0006f, 0.16f, 1000.0f, 1.0f, 1.0f, 0.07f, 0, 0, -1.5f},
    {0.0019f, 0.32f, 276.0f, 1.0f, 1.0f, 0.07f, 0, 0, -5},
    {0.0016f, 0.32f, 272.0f, 0.145f, 1.0f, 0.08f, 0, 0, -5},
    {0.016000f, 6.040000f, 0.000000f, 1.000000f, 1.000000f, 0.760000f, 0, 0,
     -50},
};

int numDemos = sizeof(demoParams) / sizeof(NBodyParams);
bool cycleDemo = true;
int activeDemo = 0;
float demoTime = 10000.0f;  // ms
StopWatchInterface *demoTimer = NULL, *timer = NULL;

// run multiple iterations to compute an average sort time

NBodyParams activeParams = demoParams[activeDemo];

// The UI.
ParamListGL *paramlist;  // parameter list
bool bShowSliders = true;

// fps
static int fpsCount = 0;
static int fpsLimit = 5;

template <typename T>
class NBodyDemo {
 public:
  static void Create() { m_singleton = new NBodyDemo; }
  static void Destroy() { delete m_singleton; }

  static void init(int numBodies, int numDevices, int blockSize, bool usePBO,
                   bool useHostMem, bool useP2P, bool useCpu, int devID) {
    m_singleton->_init(numBodies, numDevices, blockSize, usePBO, useHostMem,
                       useP2P, useCpu, devID);
  }

  static void reset(int numBodies, NBodyConfig config) {
    m_singleton->_reset(numBodies, config);
  }

  static void selectDemo(int index) { m_singleton->_selectDemo(index); }

  static bool compareResults(int numBodies) {
    return m_singleton->_compareResults(numBodies);
  }

  static void runBenchmark(int iterations) {
    m_singleton->_runBenchmark(iterations);
  }

  static void updateParams() {
    m_singleton->m_nbody->setSoftening(activeParams.m_softening);
    m_singleton->m_nbody->setDamping(activeParams.m_damping);
  }

  static void updateSimulation() {
    m_singleton->m_nbody->update(activeParams.m_timestep);
  }

  static void display() {
    m_singleton->m_renderer->setSpriteSize(activeParams.m_pointSize);

    if (useHostMem) {
      // This event sync is required because we are rendering from the host
      // memory that CUDA is
      // writing.  If we don't wait until CUDA is done updating it, we will
      // render partially
      // updated data, resulting in a jerky frame rate.
      m_singleton->m_renderer->setPositions(
          m_singleton->m_nbody->getArray(BODYSYSTEM_POSITION),
          m_singleton->m_nbody->getNumBodies());
    } else {
      m_singleton->m_renderer->setPBO(
          m_singleton->m_nbody->getCurrentReadBuffer(),
          m_singleton->m_nbody->getNumBodies(), (sizeof(T) > 4));
    }

    // display particles
    m_singleton->m_renderer->display(displayMode);
  }

  static void getArrays(T *pos, T *vel) {
    T *_pos = m_singleton->m_nbody->getArray(BODYSYSTEM_POSITION);
    T *_vel = m_singleton->m_nbody->getArray(BODYSYSTEM_VELOCITY);
    memcpy(pos, _pos, m_singleton->m_nbody->getNumBodies() * 4 * sizeof(T));
    memcpy(vel, _vel, m_singleton->m_nbody->getNumBodies() * 4 * sizeof(T));
  }

  static void setArrays(const T *pos, const T *vel) {
    if (pos != m_singleton->m_hPos) {
      memcpy(m_singleton->m_hPos, pos, numBodies * 4 * sizeof(T));
    }

    if (vel != m_singleton->m_hVel) {
      memcpy(m_singleton->m_hVel, vel, numBodies * 4 * sizeof(T));
    }

    m_singleton->m_nbody->setArray(BODYSYSTEM_POSITION, m_singleton->m_hPos);
    m_singleton->m_nbody->setArray(BODYSYSTEM_VELOCITY, m_singleton->m_hVel);

    if (!benchmark && !useCpu && !compareToCPU) {
      m_singleton->_resetRenderer();
    }
  }

 private:
  static NBodyDemo *m_singleton;

  BodySystem<T> *m_nbody;
  BodySystemCPU<T> *m_nbodyCpu;

  ParticleRenderer *m_renderer;

  T *m_hPos;
  T *m_hVel;
  float *m_hColor;

 private:
  NBodyDemo()
      : m_nbody(0),
        m_nbodyCpu(0),
        m_renderer(0),
        m_hPos(0),
        m_hVel(0),
        m_hColor(0) {}

  ~NBodyDemo() {
    if (m_nbodyCpu) {
      delete m_nbodyCpu;
    }

    if (m_hPos) {
      delete[] m_hPos;
    }

    if (m_hVel) {
      delete[] m_hVel;
    }

    if (m_hColor) {
      delete[] m_hColor;
    }

    sdkDeleteTimer(&demoTimer);

    if (!benchmark && !compareToCPU) delete m_renderer;
  }

  void _init(int numBodies, int numDevices, int blockSize, bool bUsePBO,
             bool useHostMem, bool useP2P, bool useCpu, int devID) {
    m_nbodyCpu = new BodySystemCPU<T>(numBodies);
    m_nbody = m_nbodyCpu;

    // allocate host memory
    m_hPos = new T[numBodies * 4];
    m_hVel = new T[numBodies * 4];
    m_hColor = new float[numBodies * 4];

    m_nbody->setSoftening(activeParams.m_softening);
    m_nbody->setDamping(activeParams.m_damping);

    sdkCreateTimer(&timer);
    sdkStartTimer(&timer);

    if (!benchmark && !compareToCPU) {
      m_renderer = new ParticleRenderer;
      _resetRenderer();
    }

    sdkCreateTimer(&demoTimer);
    sdkStartTimer(&demoTimer);
  }

  void _reset(int numBodies, NBodyConfig config) {
    if (tipsyFile == "") {
      randomizeBodies(config, m_hPos, m_hVel, m_hColor,
                      activeParams.m_clusterScale, activeParams.m_velocityScale,
                      numBodies, true);
      setArrays(m_hPos, m_hVel);
    } else {
      m_nbody->loadTipsyFile(tipsyFile);
      ::numBodies = m_nbody->getNumBodies();
    }
  }

  void _resetRenderer() {
    if (fp64) {
      float color[4] = {0.4f, 0.8f, 0.1f, 1.0f};
      m_renderer->setBaseColor(color);
    } else {
      float color[4] = {1.0f, 0.6f, 0.3f, 1.0f};
      m_renderer->setBaseColor(color);
    }

    m_renderer->setColors(m_hColor, m_nbody->getNumBodies());
    m_renderer->setSpriteSize(activeParams.m_pointSize);
  }

  void _selectDemo(int index) {
    assert(index < numDemos);

    activeParams = demoParams[index];
    camera_trans[0] = camera_trans_lag[0] = activeParams.m_x;
    camera_trans[1] = camera_trans_lag[1] = activeParams.m_y;
    camera_trans[2] = camera_trans_lag[2] = activeParams.m_z;
    reset(numBodies, NBODY_CONFIG_SHELL);
    sdkResetTimer(&demoTimer);
  }

  bool _compareResults(int numBodies) {
    bool passed = true;

    m_nbody->update(0.001f);

    {
      m_nbodyCpu = new BodySystemCPU<T>(numBodies);

      m_nbodyCpu->setArray(BODYSYSTEM_POSITION, m_hPos);
      m_nbodyCpu->setArray(BODYSYSTEM_VELOCITY, m_hVel);

      m_nbodyCpu->update(0.001f);

      T *cpuPos = m_nbodyCpu->getArray(BODYSYSTEM_POSITION);

      T tolerance = 0.0005f;
    }
    if (passed) {
      printf("  OK\n");
    }
    return passed;
  }

  void _runBenchmark(int iterations) {
    // once without timing to prime the device
    if (!useCpu) {
      m_nbody->update(activeParams.m_timestep);
    }

    sdkCreateTimer(&timer);
    sdkStartTimer(&timer);

    for (int i = 0; i < iterations; ++i) {
      m_nbody->update(activeParams.m_timestep);
    }

    float milliseconds = 0;

    sdkStopTimer(&timer);
    milliseconds = sdkGetTimerValue(&timer);
    sdkStartTimer(&timer);

    double interactionsPerSecond = 0;
    double gflops = 0;
    computePerfStats(interactionsPerSecond, gflops, milliseconds, iterations);

    printf("%d bodies, total time for %d iterations: %.3f ms\n", numBodies,
           iterations, milliseconds);
    printf("= %.3f billion interactions per second\n", interactionsPerSecond);
    printf("= %.3f %s-precision GFLOP/s at %d flops per interaction\n", gflops,
           (sizeof(T) > 4) ? "double" : "single", flopsPerInteraction);
  }
};

void finalize() {
  NBodyDemo<float>::Destroy();
  if (bSupportDouble) NBodyDemo<double>::Destroy();
}

template <>
NBodyDemo<double> *NBodyDemo<double>::m_singleton = 0;
template <>
NBodyDemo<float> *NBodyDemo<float>::m_singleton = 0;

template <typename T_new, typename T_old>
void switchDemoPrecision() {
  fp64 = !fp64;
  flopsPerInteraction = fp64 ? 30 : 20;

  auto *oldPos = new T_old[numBodies * 4];
  auto *oldVel = new T_old[numBodies * 4];

  NBodyDemo<T_old>::getArrays(oldPos, oldVel);

  // convert float to double
  auto *newPos = new T_new[numBodies * 4];
  auto *newVel = new T_new[numBodies * 4];

  for (int i = 0; i < numBodies * 4; i++) {
    newPos[i] = (T_new)oldPos[i];
    newVel[i] = (T_new)oldVel[i];
  }

  NBodyDemo<T_new>::setArrays(newPos, newVel);

  delete[] oldPos;
  delete[] oldVel;
  delete[] newPos;
  delete[] newVel;
}

// check for OpenGL errors
inline void checkGLErrors(const char *s) {
  GLenum error;

  while ((error = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "%s: error - %s\n", s, (char *)gluErrorString(error));
  }
}

void initGL(int *argc, char **argv) {
  // First initialize OpenGL context, so we can properly set the GL for CUDA.
  // This is necessary in order to achieve optimal performance with OpenGL/CUDA
  // interop.
  glutInit(argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
  glutInitWindowSize(720, 480);
  glutCreateWindow("CUDA n-body system");

  if (bFullscreen) {
    glutFullScreen();
  } else {
#if defined(LINUX)
    glxSwapIntervalSGI(0);
#endif
  }

  glEnable(GL_DEPTH_TEST);
  glClearColor(0.0, 0.0, 0.0, 1.0);

  checkGLErrors("initGL");
}

void initParameters() {
  // create a new parameter list
  paramlist = new ParamListGL("sliders");
  paramlist->SetBarColorInner(0.8f, 0.8f, 0.0f);

  // add some parameters to the list

  // Point Size
  paramlist->AddParam(new Param<float>("Point Size", activeParams.m_pointSize,
                                       0.001f, 10.0f, 0.01f,
                                       &activeParams.m_pointSize));

  // Velocity Damping
  paramlist->AddParam(new Param<float>("Velocity Damping",
                                       activeParams.m_damping, 0.5f, 1.0f,
                                       .0001f, &(activeParams.m_damping)));
  // Softening Factor
  paramlist->AddParam(new Param<float>("Softening Factor",
                                       activeParams.m_softening, 0.001f, 1.0f,
                                       .0001f, &(activeParams.m_softening)));
  // Time step size
  paramlist->AddParam(new Param<float>("Time Step", activeParams.m_timestep,
                                       0.0f, 1.0f, .0001f,
                                       &(activeParams.m_timestep)));
  // Cluster scale (only affects starting configuration
  paramlist->AddParam(new Param<float>("Cluster Scale",
                                       activeParams.m_clusterScale, 0.0f, 10.0f,
                                       0.01f, &(activeParams.m_clusterScale)));

  // Velocity scale (only affects starting configuration)
  paramlist->AddParam(
      new Param<float>("Velocity Scale", activeParams.m_velocityScale, 0.0f,
                       1000.0f, 0.1f, &activeParams.m_velocityScale));
}

void selectDemo(int activeDemo) {
  if (fp64) {
    NBodyDemo<double>::selectDemo(activeDemo);
  } else {
    NBodyDemo<float>::selectDemo(activeDemo);
  }
}

void updateSimulation() {
  if (fp64) {
    NBodyDemo<double>::updateSimulation();
  } else {
    NBodyDemo<float>::updateSimulation();
  }
}

void displayNBodySystem() {
  if (fp64) {
    NBodyDemo<double>::display();
  } else {
    NBodyDemo<float>::display();
  }
}

void display() {
  static double gflops = 0;
  static double ifps = 0;
  static double interactionsPerSecond = 0;

  // update the simulation
  if (!bPause) {
    if (cycleDemo && (sdkGetTimerValue(&demoTimer) > demoTime)) {
      activeDemo = (activeDemo + 1) % numDemos;
      selectDemo(activeDemo);
    }

    updateSimulation();
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (displayEnabled) {
    // view transform
    {
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();

      for (int c = 0; c < 3; ++c) {
        camera_trans_lag[c] +=
            (camera_trans[c] - camera_trans_lag[c]) * inertia;
        camera_rot_lag[c] += (camera_rot[c] - camera_rot_lag[c]) * inertia;
      }

      glTranslatef(camera_trans_lag[0], camera_trans_lag[1],
                   camera_trans_lag[2]);
      glRotatef(camera_rot_lag[0], 1.0, 0.0, 0.0);
      glRotatef(camera_rot_lag[1], 0.0, 1.0, 0.0);
    }

    displayNBodySystem();

    // display user interface
    if (bShowSliders) {
      glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);  // invert color
      glEnable(GL_BLEND);
      paramlist->Render(0, 0);
      glDisable(GL_BLEND);
    }

    if (bFullscreen) {
      beginWinCoords();
      char msg0[256], msg1[256], msg2[256];

      if (bDispInteractions) {
        sprintf(msg1, "%0.2f billion interactions per second",
                interactionsPerSecond);
      } else {
        sprintf(msg1, "%0.2f GFLOP/s", gflops);
      }

      sprintf(msg0, "%s", deviceName);
      sprintf(msg2, "%0.2f FPS [%s | %d bodies]", ifps,
              fp64 ? "double precision" : "single precision", numBodies);

      glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);  // invert color
      glEnable(GL_BLEND);
      glColor3f(0.46f, 0.73f, 0.0f);
      glPrint(80, glutGet(GLUT_WINDOW_HEIGHT) - 122, msg0,
              GLUT_BITMAP_TIMES_ROMAN_24);
      glColor3f(1.0f, 1.0f, 1.0f);
      glPrint(80, glutGet(GLUT_WINDOW_HEIGHT) - 96, msg2,
              GLUT_BITMAP_TIMES_ROMAN_24);
      glColor3f(1.0f, 1.0f, 1.0f);
      glPrint(80, glutGet(GLUT_WINDOW_HEIGHT) - 70, msg1,
              GLUT_BITMAP_TIMES_ROMAN_24);
      glDisable(GL_BLEND);

      endWinCoords();
    }

    glutSwapBuffers();
  }

  fpsCount++;

  // this displays the frame rate updated every second (independent of frame
  // rate)
  if (fpsCount >= fpsLimit) {
    char fps[256];

    float milliseconds = 1;

    // stop timer
    milliseconds = sdkGetTimerValue(&timer);
    sdkResetTimer(&timer);

    milliseconds /= (float)fpsCount;
    computePerfStats(interactionsPerSecond, gflops, milliseconds, 1);

    ifps = 1.f / (milliseconds / 1000.f);
    sprintf(fps,
            "CUDA N-Body (%d bodies): "
            "%0.1f fps | %0.1f BIPS | %0.1f GFLOP/s | %s",
            numBodies, ifps, interactionsPerSecond, gflops,
            fp64 ? "double precision" : "single precision");

    glutSetWindowTitle(fps);
    fpsCount = 0;
    fpsLimit = (ifps > 1.f) ? (int)ifps : 1;

    if (bPause) {
      fpsLimit = 0;
    }
  }

  glutReportErrors();
}

void reshape(int w, int h) {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0, (float)w / (float)h, 0.1, 1000.0);

  glMatrixMode(GL_MODELVIEW);
  glViewport(0, 0, w, h);
}

void updateParams() {
  if (fp64) {
    NBodyDemo<double>::updateParams();
  } else {
    NBodyDemo<float>::updateParams();
  }
}

void mouse(int button, int state, int x, int y) {
  if (bShowSliders) {
    // call list mouse function
    if (paramlist->Mouse(x, y, button, state)) {
      updateParams();
    }
  }

  int mods;

  if (state == GLUT_DOWN) {
    buttonState |= 1 << button;
  } else if (state == GLUT_UP) {
    buttonState = 0;
  }

  mods = glutGetModifiers();

  if (mods & GLUT_ACTIVE_SHIFT) {
    buttonState = 2;
  } else if (mods & GLUT_ACTIVE_CTRL) {
    buttonState = 3;
  }

  ox = x;
  oy = y;

  glutPostRedisplay();
}

void motion(int x, int y) {
  if (bShowSliders) {
    // call parameter list motion function
    if (paramlist->Motion(x, y)) {
      updateParams();
      glutPostRedisplay();
      return;
    }
  }

  float dx = (float)(x - ox);
  float dy = (float)(y - oy);

  if (buttonState == 3) {
    // left+middle = zoom
    camera_trans[2] += (dy / 100.0f) * 0.5f * fabs(camera_trans[2]);
  } else if (buttonState & 2) {
    // middle = translate
    camera_trans[0] += dx / 100.0f;
    camera_trans[1] -= dy / 100.0f;
  } else if (buttonState & 1) {
    // left = rotate
    camera_rot[0] += dy / 5.0f;
    camera_rot[1] += dx / 5.0f;
  }

  ox = x;
  oy = y;
  glutPostRedisplay();
}

// commented out to remove unused parameter warnings in Linux
void key(unsigned char key, int /*x*/, int /*y*/) {
  switch (key) {
    case ' ':
      bPause = !bPause;
      break;

    case 27:  // escape
    case 'q':
    case 'Q':
      finalize();
      exit(EXIT_SUCCESS);
      break;

    case 13:  // return
      if (bSupportDouble) {
        if (fp64) {
          switchDemoPrecision<float, double>();
        } else {
          switchDemoPrecision<double, float>();
        }

        printf("> %s precision floating point simulation\n",
               fp64 ? "Double" : "Single");
      }

      break;

    case '`':
      bShowSliders = !bShowSliders;
      break;

    case 'g':
    case 'G':
      bDispInteractions = !bDispInteractions;
      break;

    case 'p':
    case 'P':
      displayMode = (ParticleRenderer::DisplayMode)(
          (displayMode + 1) % ParticleRenderer::PARTICLE_NUM_MODES);
      break;

    case 'c':
    case 'C':
      cycleDemo = !cycleDemo;
      printf("Cycle Demo Parameters: %s\n", cycleDemo ? "ON" : "OFF");
      break;

    case '[':
      activeDemo =
          (activeDemo == 0) ? numDemos - 1 : (activeDemo - 1) % numDemos;
      selectDemo(activeDemo);
      break;

    case ']':
      activeDemo = (activeDemo + 1) % numDemos;
      selectDemo(activeDemo);
      break;

    case 'd':
    case 'D':
      displayEnabled = !displayEnabled;
      break;

    case 'o':
    case 'O':
      activeParams.print();
      break;

    case '1':
      if (fp64) {
        NBodyDemo<double>::reset(numBodies, NBODY_CONFIG_SHELL);
      } else {
        NBodyDemo<float>::reset(numBodies, NBODY_CONFIG_SHELL);
      }

      break;

    case '2':
      if (fp64) {
        NBodyDemo<double>::reset(numBodies, NBODY_CONFIG_RANDOM);
      } else {
        NBodyDemo<float>::reset(numBodies, NBODY_CONFIG_RANDOM);
      }

      break;

    case '3':
      if (fp64) {
        NBodyDemo<double>::reset(numBodies, NBODY_CONFIG_EXPAND);
      } else {
        NBodyDemo<float>::reset(numBodies, NBODY_CONFIG_EXPAND);
      }

      break;
  }

  glutPostRedisplay();
}

void special(int key, int x, int y) {
  paramlist->Special(key, x, y);
  glutPostRedisplay();
}

void idle(void) { glutPostRedisplay(); }

void showHelp() {
  printf("\t-fullscreen       (run n-body simulation in fullscreen mode)\n");
  printf(
      "\t-fp64             (use double precision floating point values for "
      "simulation)\n");
  printf("\t-hostmem          (stores simulation data in host memory)\n");
  printf("\t-benchmark        (run benchmark to measure performance) \n");
  printf(
      "\t-numbodies=<N>    (number of bodies (>= 1) to run in simulation) \n");
  printf(
      "\t-device=<d>       (where d=0,1,2.... for the CUDA device to use)\n");
  printf(
      "\t-numdevices=<i>   (where i=(number of CUDA devices > 0) to use for "
      "simulation)\n");
  printf(
      "\t-compare          (compares simulation results running once on the "
      "default GPU and once on the CPU)\n");
  printf("\t-cpu              (run n-body simulation on the CPU)\n");
  printf("\t-tipsy=<file.bin> (load a tipsy model file for simulation)\n\n");
}

////////////////////////////////////////////////////////////////////////////////
// Program main
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
  bool bTestResults = true;

  setenv("DISPLAY", ":0", 0);

  bFullscreen = true;
  if (bFullscreen) {
    bShowSliders = false;
  }


  compareToCPU = true;
  flopsPerInteraction = fp64 ? 30 : 20;
  useCpu = true;

  int numDevsAvailable = 0;
  bool customGPU = false;

  printf("> %s mode\n", bFullscreen ? "Fullscreen" : "Windowed");
  printf("> Simulation data stored in %s memory\n",
         useHostMem ? "system" : "video");
  printf("> %s precision floating point simulation\n",
         fp64 ? "Double" : "Single");
  printf("> %d Devices used for simulation\n", numDevsRequested);

  useHostMem = true;
  compareToCPU = false;
  bSupportDouble = true;
  printf("> Simulation with CPU\n");

  // Initialize GL and GLUT if necessary
  initGL(&argc, argv);
  initParameters();

  numIterations = 1;
  blockSize = 256;
  // default number of bodies is #SMs * 4 * CTA size
  if (useCpu)
    numBodies = 4096;

  printf("number of bodies = %d\n", numBodies);

  char *fname;

  if (numBodies <= 1024) {
    activeParams.m_clusterScale = 1.52f;
    activeParams.m_velocityScale = 2.f;
  } else if (numBodies <= 2048) {
    activeParams.m_clusterScale = 1.56f;
    activeParams.m_velocityScale = 2.64f;
  } else if (numBodies <= 4096) {
    activeParams.m_clusterScale = 1.68f;
    activeParams.m_velocityScale = 2.98f;
  } else if (numBodies <= 8192) {
    activeParams.m_clusterScale = 1.98f;
    activeParams.m_velocityScale = 2.9f;
  } else if (numBodies <= 16384) {
    activeParams.m_clusterScale = 1.54f;
    activeParams.m_velocityScale = 8.f;
  } else if (numBodies <= 32768) {
    activeParams.m_clusterScale = 1.44f;
    activeParams.m_velocityScale = 11.f;
  }

  // Create the demo -- either double (fp64) or float (fp32, default)
  // implementation
  NBodyDemo<float>::Create();

  NBodyDemo<float>::init(numBodies, numDevsRequested, blockSize,
                         !(benchmark || compareToCPU || useHostMem), useHostMem,
                         useP2P, useCpu, 0);
  NBodyDemo<float>::reset(numBodies, NBODY_CONFIG_SHELL);

  if (bSupportDouble) {
    NBodyDemo<double>::Create();
    NBodyDemo<double>::init(numBodies, numDevsRequested, blockSize,
                            !(benchmark || compareToCPU || useHostMem),
                            useHostMem, useP2P, useCpu, 0);
    NBodyDemo<double>::reset(numBodies, NBODY_CONFIG_SHELL);
  }

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);
    glutIdleFunc(idle);

    glutMainLoop();

  finalize();

  return 0;
}
