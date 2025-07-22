/*
MIT License

Copyright (c) 2025 Expert Sleepers Ltd
(Portions adapted from mod_julia.lua by Thorinside)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <math.h>
#include <new>
#include <stdlib.h> // For rand, srand
#include <distingnt/api.h>

// --- Constants ---
const float JULIA_C_REAL = -0.7f;
const float JULIA_C_IMAG = 0.27015f;
const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 64;
const int JULIA_RECT_W = 2;
const int JULIA_RECT_H = 2;
const int NUM_JULIA_RECTS_X = SCREEN_WIDTH / JULIA_RECT_W;
const int NUM_JULIA_RECTS_Y = SCREEN_HEIGHT / JULIA_RECT_H;

const float BALL_RECT_W_F = 5.0f;
const float BALL_RECT_H_F = 5.0f;
const float BALL_HALF_W_F = BALL_RECT_W_F / 2.0f;
const float BALL_HALF_H_F = BALL_RECT_H_F / 2.0f;
const uint8_t BALL_OUTLINE_COLOR = 15; // White

const int iteration_values[] = {32, 64, 128, 256, 512, 1024};
char const *const enumStringsIterations[] = {"32", "64", "128", "256", "512", "1024", NULL};

// --- Data Structures ---
struct _ModJuliaAlgorithm_DTC
{
  // Julia properties
  float c_real;
  float c_imag;
  int max_iterations;
  float zoom;
  float offset_x;
  float offset_y;

  uint8_t julia_rect_colors[NUM_JULIA_RECTS_X * NUM_JULIA_RECTS_Y];

  // Ball physics
  float ball_x;
  float ball_y;
  float ball_vx;
  float ball_vy;

  float base_ball_vx;
  float base_ball_vy;
  float base_speed_magnitude;
  float base_dir_x;
  float base_dir_y;

  uint8_t ball_draw_color;
  float current_output_voltage;
};

struct _ModJuliaAlgorithm : public _NT_algorithm
{
  _ModJuliaAlgorithm_DTC *dtc;
  bool needs_julia_recalc;
  int16_t lastResetTriggerVal;
  bool cvResetInputHigh; // Added to track CV Reset input state

  _ModJuliaAlgorithm(_ModJuliaAlgorithm_DTC *dtc_ptr) : dtc(dtc_ptr), needs_julia_recalc(true), lastResetTriggerVal(0), cvResetInputHigh(false) {}

  void resetBallPriv(int current_ball_speed_param_val);
  void updateBallSpeedPriv(int current_ball_speed_param_val);
  void calculateJuliaRectsPriv();
  int calculateJuliaIterations(float zx, float zy);
};

// --- Parameter Definitions ---
enum
{
  kParamOutput,
  kParamOutputMode,
  kParamIterations,
  kParamZoom,
  kParamOffsetX,
  kParamOffsetY,
  kParamBallSpeed,
  kParamResetTrigger,
  kParamCvResetIn, // Added new CV Reset Input parameter enum
  // Add kParamResetTrigger if physical input trigger is desired
};

_NT_parameter parameters[] = {
    NT_PARAMETER_CV_OUTPUT_WITH_MODE("Mod Out", 1, 13)                                                                                                             // Output, OutputMode
    {.name = "Iterations", .min = 0, .max = ARRAY_SIZE(iteration_values) - 1, .def = 3, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = enumStringsIterations}, // Default 256 (index 3)
    {.name = "Zoom", .min = 1, .max = 1000, .def = 100, .unit = kNT_unitNone, .scaling = kNT_scaling10},                                                           // Lua default 1, here 1.0 (scaled by 10) -> 10
    {.name = "Offset X", .min = -1000, .max = 1000, .def = 0, .unit = kNT_unitNone, .scaling = kNT_scaling100},                                                    // Val / 1000.0. Lua def 0
    {.name = "Offset Y", .min = -1000, .max = 1000, .def = 0, .unit = kNT_unitNone, .scaling = kNT_scaling100},                                                    // Val / 1000.0. Lua def 0
    {.name = "Ball Speed", .min = 10, .max = 200, .def = 100, .unit = kNT_unitPercent, .scaling = 0},                                                              // Lua def 100
    {.name = "Reset Ball", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = 0, .enumStrings = (char const *const[]){(char *)"Off", (char *)"Trigger", NULL}},
    NT_PARAMETER_CV_INPUT("CV Reset In", 0, 0) // Added CV Reset Input parameter, default to no input (0)
};

uint8_t page1[] = {kParamIterations, kParamZoom, kParamBallSpeed, kParamResetTrigger};
uint8_t page2[] = {kParamOffsetX, kParamOffsetY};
uint8_t page3[] = {kParamCvResetIn, kParamOutput, kParamOutputMode};

_NT_parameterPage pages[] = {
    {.name = "Julia", .numParams = ARRAY_SIZE(page1), .params = page1},
    {.name = "Offsets", .numParams = ARRAY_SIZE(page2), .params = page2},
    {.name = "Routing", .numParams = ARRAY_SIZE(page3), .params = page3},
};

_NT_parameterPages parameterPages = {
    .numPages = ARRAY_SIZE(pages),
    .pages = pages,
};

// --- Algorithm Functions ---
void calculateRequirements(_NT_algorithmRequirements &req, const int32_t *specifications)
{
  req.numParameters = ARRAY_SIZE(parameters);
  req.sram = sizeof(_ModJuliaAlgorithm);
  req.dram = 0;
  req.dtc = sizeof(_ModJuliaAlgorithm_DTC);
  req.itc = 0;
}

_NT_algorithm *construct(const _NT_algorithmMemoryPtrs &ptrs, const _NT_algorithmRequirements &req, const int32_t *specifications)
{
  _ModJuliaAlgorithm_DTC *dtc = new (ptrs.dtc) _ModJuliaAlgorithm_DTC();
  _ModJuliaAlgorithm *alg = new (ptrs.sram) _ModJuliaAlgorithm(dtc);

  alg->parameters = parameters;
  alg->parameterPages = &parameterPages;

  // Initialize DTC
  dtc->c_real = JULIA_C_REAL;
  dtc->c_imag = JULIA_C_IMAG;

  // Default param values will be set by host, apply them initially
  // Or call parameterChanged for all relevant params to init dtc->julia_props and ball speed props

  dtc->base_ball_vx = 50.0f;
  dtc->base_ball_vy = 30.0f;
  dtc->base_speed_magnitude = sqrtf(dtc->base_ball_vx * dtc->base_ball_vx + dtc->base_ball_vy * dtc->base_ball_vy);
  if (dtc->base_speed_magnitude > 0.0001f)
  {
    dtc->base_dir_x = dtc->base_ball_vx / dtc->base_speed_magnitude;
    dtc->base_dir_y = dtc->base_ball_vy / dtc->base_speed_magnitude;
  }
  else
  {
    dtc->base_dir_x = 0.0f; // Or 1.0f for default direction if base speed is zero
    dtc->base_dir_y = 0.0f;
  }

  // Seed random number generator
  srand((unsigned int)NT_getCpuCycleCount());

  // Initial parameter application
  // Max Iterations
  int iter_idx = alg->v[kParamIterations]; // Assuming v is populated by now
  if (iter_idx >= 0 && iter_idx < (int)ARRAY_SIZE(iteration_values))
  {
    dtc->max_iterations = iteration_values[iter_idx];
  }
  else
  {
    dtc->max_iterations = iteration_values[parameters[kParamIterations].def];
  }
  // Zoom
  dtc->zoom = (float)alg->v[kParamZoom] / powf(10.0f, parameters[kParamZoom].scaling); // Or use default if v not ready
  if (dtc->zoom < 0.001f)
    dtc->zoom = 0.001f; // Prevent division by zero, ensure positive
  // Offset X
  dtc->offset_x = (float)alg->v[kParamOffsetX] / powf(10.0f, parameters[kParamOffsetX].scaling);
  // Offset Y
  dtc->offset_y = (float)alg->v[kParamOffsetY] / powf(10.0f, parameters[kParamOffsetY].scaling);

  alg->resetBallPriv(alg->v[kParamBallSpeed]); // Initial ball state based on default speed
  alg->needs_julia_recalc = true;              // Ensure Julia set is calculated on first step

  return alg;
}

int _ModJuliaAlgorithm::calculateJuliaIterations(float zx, float zy)
{
  int iter = 0;
  float cr = dtc->c_real;
  float ci = dtc->c_imag;
  while (zx * zx + zy * zy <= 4.0f && iter < dtc->max_iterations)
  {
    float xtemp = zx * zx - zy * zy + cr;
    zy = 2.0f * zx * zy + ci;
    zx = xtemp;
    iter++;
  }
  return iter;
}

void _ModJuliaAlgorithm::calculateJuliaRectsPriv()
{
  for (int ry = 0; ry < NUM_JULIA_RECTS_Y; ++ry)
  {
    for (int rx = 0; rx < NUM_JULIA_RECTS_X; ++rx)
    {
      float screen_x_center = (float)rx * JULIA_RECT_W + JULIA_RECT_W / 2.0f;
      float screen_y_center = (float)ry * JULIA_RECT_H + JULIA_RECT_H / 2.0f;

      float zx = (screen_x_center / SCREEN_WIDTH) * (4.0f / dtc->zoom) - (2.0f / dtc->zoom) + dtc->offset_x;
      float zy = (screen_y_center / SCREEN_HEIGHT) * (2.0f / dtc->zoom) - (1.0f / dtc->zoom) + dtc->offset_y; // Adjusted for typical Julia aspect ratio

      int iterations = calculateJuliaIterations(zx, zy);
      uint8_t color;
      if (iterations == dtc->max_iterations)
      {
        color = 0; // Inside the set: black
      }
      else
      {
        color = (iterations % 15) + 1; // Outside: cycling colors 1-15
      }
      dtc->julia_rect_colors[ry * NUM_JULIA_RECTS_X + rx] = color;
    }
  }
  needs_julia_recalc = false;
}

void _ModJuliaAlgorithm::resetBallPriv(int current_ball_speed_param_val)
{
  dtc->ball_x = SCREEN_WIDTH / 2.0f - BALL_HALF_W_F;
  dtc->ball_y = SCREEN_HEIGHT / 2.0f - BALL_HALF_H_F;

  float rand_dir_x, rand_dir_y;
  do
  {
    rand_dir_x = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    rand_dir_y = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
  } while (rand_dir_x == 0.0f && rand_dir_y == 0.0f);

  float dir_magnitude = sqrtf(rand_dir_x * rand_dir_x + rand_dir_y * rand_dir_y);
  rand_dir_x /= dir_magnitude;
  rand_dir_y /= dir_magnitude;

  updateBallSpeedPriv(current_ball_speed_param_val); // Sets vx, vy based on current speed param
  // After updateBallSpeedPriv, if ball is stationary, apply random direction with new speed
  if (fabsf(dtc->ball_vx) < 0.0001f && fabsf(dtc->ball_vy) < 0.0001f)
  {
    float speed_multiplier = (float)current_ball_speed_param_val / 100.0f;
    float new_total_speed = dtc->base_speed_magnitude * speed_multiplier;
    if (dtc->base_speed_magnitude <= 0.0001f)
    { // If base speed is zero, use a default speed magnitude
      new_total_speed = 50.0f * speed_multiplier;
    }
    dtc->ball_vx = rand_dir_x * new_total_speed;
    dtc->ball_vy = rand_dir_y * new_total_speed;
  }
  else
  { // If ball was moving, updateBallSpeedPriv already scaled it. Now apply random direction.
    float current_speed = sqrtf(dtc->ball_vx * dtc->ball_vx + dtc->ball_vy * dtc->ball_vy);
    dtc->ball_vx = rand_dir_x * current_speed;
    dtc->ball_vy = rand_dir_y * current_speed;
  }
}

void _ModJuliaAlgorithm::updateBallSpeedPriv(int current_ball_speed_param_val)
{
  float speed_multiplier = (float)current_ball_speed_param_val / 100.0f;
  float new_total_speed = dtc->base_speed_magnitude * speed_multiplier;

  float current_vel_magnitude_sq = dtc->ball_vx * dtc->ball_vx + dtc->ball_vy * dtc->ball_vy;

  if (current_vel_magnitude_sq > 0.0001f)
  { // If ball is moving, preserve direction
    float current_vel_magnitude = sqrtf(current_vel_magnitude_sq);
    float dir_x = dtc->ball_vx / current_vel_magnitude;
    float dir_y = dtc->ball_vy / current_vel_magnitude;
    dtc->ball_vx = dir_x * new_total_speed;
    dtc->ball_vy = dir_y * new_total_speed;
  }
  else
  { // If ball is stationary, apply new speed to base direction
    if (dtc->base_speed_magnitude > 0.0001f)
    {
      dtc->ball_vx = dtc->base_dir_x * new_total_speed;
      dtc->ball_vy = dtc->base_dir_y * new_total_speed;
    }
    else
    { // Base speed is also zero
      dtc->ball_vx = 0.0f;
      dtc->ball_vy = 0.0f;
    }
  }
}

void parameterChanged(_NT_algorithm *self_base, int p)
{
  _ModJuliaAlgorithm *self = (_ModJuliaAlgorithm *)self_base;
  _ModJuliaAlgorithm_DTC *dtc = self->dtc;

  bool param_affects_julia = false;

  if (p == kParamIterations)
  {
    int iter_idx = self->v[kParamIterations];
    if (iter_idx >= 0 && iter_idx < (int)ARRAY_SIZE(iteration_values))
    {
      dtc->max_iterations = iteration_values[iter_idx];
      param_affects_julia = true;
    }
  }
  else if (p == kParamZoom)
  {
    float new_zoom = (float)self->v[kParamZoom] / powf(10.0f, parameters[kParamZoom].scaling);
    if (new_zoom < 0.001f)
      new_zoom = 0.001f; // prevent div by zero
    if (fabsf(dtc->zoom - new_zoom) > 0.0001f)
    {
      dtc->zoom = new_zoom;
      param_affects_julia = true;
    }
  }
  else if (p == kParamOffsetX)
  {
    float new_offset_x = (float)self->v[kParamOffsetX] / powf(10.0f, parameters[kParamOffsetX].scaling);
    if (fabsf(dtc->offset_x - new_offset_x) > 0.0001f)
    {
      dtc->offset_x = new_offset_x;
      param_affects_julia = true;
    }
  }
  else if (p == kParamOffsetY)
  {
    float new_offset_y = (float)self->v[kParamOffsetY] / powf(10.0f, parameters[kParamOffsetY].scaling);
    if (fabsf(dtc->offset_y - new_offset_y) > 0.0001f)
    {
      dtc->offset_y = new_offset_y;
      param_affects_julia = true;
    }
  }
  else if (p == kParamBallSpeed)
  {
    self->updateBallSpeedPriv(self->v[kParamBallSpeed]);
  }
  else if (p == kParamResetTrigger)
  {
    int16_t current_trigger_val = self->v[kParamResetTrigger];
    if (current_trigger_val == 1 && self->lastResetTriggerVal == 0)
    {
      self->resetBallPriv(self->v[kParamBallSpeed]);
      // Automatically set the trigger back to 0 (Off)
      NT_setParameterFromUi(NT_algorithmIndex(self), kParamResetTrigger + NT_parameterOffset(), 0);
    }
    self->lastResetTriggerVal = current_trigger_val;
  }

  if (param_affects_julia)
  {
    self->needs_julia_recalc = true;
  }
}

void step(_NT_algorithm *self_base, float *busFrames, int numFramesBy4)
{
  _ModJuliaAlgorithm *self = (_ModJuliaAlgorithm *)self_base;
  _ModJuliaAlgorithm_DTC *dtc = self->dtc;
  int numFrames = numFramesBy4 * 4;

  // Get CV Reset Input bus
  int cv_reset_bus_idx = self->v[kParamCvResetIn];
  float *cv_reset_input_bus = nullptr;
  if (cv_reset_bus_idx > 0 && cv_reset_bus_idx <= 28) // Assuming 28 is max bus number
  {
    cv_reset_input_bus = busFrames + (cv_reset_bus_idx - 1) * numFrames;
  }

  if (self->needs_julia_recalc)
  {
    self->calculateJuliaRectsPriv();
  }

  float block_dt = (float)numFrames / NT_globals.sampleRate;

  dtc->ball_x += dtc->ball_vx * block_dt;
  dtc->ball_y += dtc->ball_vy * block_dt;

  // Screen edge collision
  if (dtc->ball_x < 0)
  {
    dtc->ball_x = 0;
    dtc->ball_vx = -dtc->ball_vx;
  }
  else if (dtc->ball_x + BALL_RECT_W_F > SCREEN_WIDTH)
  {
    dtc->ball_x = SCREEN_WIDTH - BALL_RECT_W_F;
    dtc->ball_vx = -dtc->ball_vx;
  }

  if (dtc->ball_y < 0)
  {
    dtc->ball_y = 0;
    dtc->ball_vy = -dtc->ball_vy;
  }
  else if (dtc->ball_y + BALL_RECT_H_F > SCREEN_HEIGHT)
  {
    dtc->ball_y = SCREEN_HEIGHT - BALL_RECT_H_F;
    dtc->ball_vy = -dtc->ball_vy;
  }

  // Determine output color based on Julia set under ball's center
  uint8_t output_color_idx = 0;
  float ball_center_x = dtc->ball_x + BALL_HALF_W_F;
  float ball_center_y = dtc->ball_y + BALL_HALF_H_F;

  int target_rx = floorf(ball_center_x / JULIA_RECT_W);
  int target_ry = floorf(ball_center_y / JULIA_RECT_H);

  target_rx = fmaxf(0, fminf(target_rx, NUM_JULIA_RECTS_X - 1));
  target_ry = fmaxf(0, fminf(target_ry, NUM_JULIA_RECTS_Y - 1));

  output_color_idx = dtc->julia_rect_colors[target_ry * NUM_JULIA_RECTS_X + target_rx];
  dtc->ball_draw_color = output_color_idx;
  dtc->current_output_voltage = (float)output_color_idx / 15.0f * 10.0f;

  // Output to CV
  int cvOutputIdx = self->v[kParamOutput] - 1;

  // --- Audio Rate Loop Start (Conceptual - actual loop is for processing all samples) ---
  // The CV input needs to be checked per sample if it can change mid-block.
  for (int i = 0; i < numFrames; ++i)
  {
    // CV Reset Input Check
    if (cv_reset_input_bus != nullptr)
    {
      float cv_reset_sample = cv_reset_input_bus[i];
      if (cv_reset_sample > 1.0f && !self->cvResetInputHigh)
      {
        NT_setParameterFromAudio(NT_algorithmIndex(self), kParamResetTrigger + NT_parameterOffset(), 1);
        self->cvResetInputHigh = true;
      }
      else if (cv_reset_sample <= 1.0f && self->cvResetInputHigh) // Detect falling edge to reset state
      {
        self->cvResetInputHigh = false;
      }
    }

    // Original per-sample output logic (if any was here)
    // This part seems to be missing from the original snippet, assuming it's outputting a block-level value
  }
  // --- Audio Rate Loop End ---

  if (cvOutputIdx >= 0 && cvOutputIdx < 28)
  { // Max 28 busses
    float *cvOutput = busFrames + cvOutputIdx * numFrames;
    bool cvReplace = self->v[kParamOutputMode];
    for (int i = 0; i < numFrames; ++i)
    {
      if (cvReplace)
      {
        cvOutput[i] = dtc->current_output_voltage;
      }
      else
      {
        cvOutput[i] += dtc->current_output_voltage;
      }
    }
  }
}

bool draw(_NT_algorithm *self_base)
{
  _ModJuliaAlgorithm *self = (_ModJuliaAlgorithm *)self_base;
  _ModJuliaAlgorithm_DTC *dtc = self->dtc;

  // Draw Julia set
  for (int ry = 0; ry < NUM_JULIA_RECTS_Y; ++ry)
  {
    for (int rx = 0; rx < NUM_JULIA_RECTS_X; ++rx)
    {
      int screen_x = rx * JULIA_RECT_W;
      int screen_y = ry * JULIA_RECT_H;
      uint8_t color = dtc->julia_rect_colors[ry * NUM_JULIA_RECTS_X + rx];
      NT_drawShapeI(kNT_rectangle, screen_x, screen_y, screen_x + JULIA_RECT_W - 1, screen_y + JULIA_RECT_H - 1, color);
    }
  }

  // Draw bouncing ball
  int ball_draw_x = floorf(dtc->ball_x + 0.5f);
  int ball_draw_y = floorf(dtc->ball_y + 0.5f);
  int ball_x2 = ball_draw_x + (int)BALL_RECT_W_F - 1;
  int ball_y2 = ball_draw_y + (int)BALL_RECT_H_F - 1;

  NT_drawShapeI(kNT_rectangle, ball_draw_x, ball_draw_y, ball_x2, ball_y2, dtc->ball_draw_color);
  NT_drawShapeI(kNT_box, ball_draw_x, ball_draw_y, ball_x2, ball_y2, BALL_OUTLINE_COLOR);

  return true; // Hide default parameter line
}

uint32_t hasCustomUi(_NT_algorithm *self)
{
  return kNT_potL | kNT_potC | kNT_potR | kNT_encoderL | kNT_encoderR | kNT_encoderButtonR;
}

void setupUi(_NT_algorithm *self_base, _NT_float3 &pots_out)
{
  _ModJuliaAlgorithm *self = (_ModJuliaAlgorithm *)self_base;
  // Pots are 0..1 normalized

  // Iterations (Pot 1 -> pots_out[0])
  int num_iter_options = ARRAY_SIZE(iteration_values);
  if (num_iter_options > 1)
  {
    pots_out[0] = (float)self->v[kParamIterations] / (num_iter_options - 1);
  }
  else
  {
    pots_out[0] = 0.5f;
  }

  // Zoom (Pot 2 -> pots_out[1])
  const _NT_parameter &zoom_spec = parameters[kParamZoom];
  if (zoom_spec.max > zoom_spec.min)
  {
    pots_out[1] = (float)(self->v[kParamZoom] - zoom_spec.min) / (zoom_spec.max - zoom_spec.min);
  }
  else
  {
    pots_out[1] = 0.5f;
  }

  // Ball Speed (Pot 3 -> pots_out[2])
  const _NT_parameter &speed_spec = parameters[kParamBallSpeed];
  if (speed_spec.max > speed_spec.min)
  {
    pots_out[2] = (float)(self->v[kParamBallSpeed] - speed_spec.min) / (speed_spec.max - speed_spec.min);
  }
  else
  {
    pots_out[2] = 0.5f;
  }
}

void customUi(_NT_algorithm *self_base, const _NT_uiData &data)
{
  _ModJuliaAlgorithm *self = (_ModJuliaAlgorithm *)self_base;
  uint32_t algIdx = NT_algorithmIndex(self);
  uint32_t paramOffset = NT_parameterOffset();

  // Pots (L, C, R) -> Parameters (Iterations, Zoom, Ball Speed)
  if (data.controls & kNT_potL)
  { // Iterations
    const _NT_parameter &iter_spec = parameters[kParamIterations];
    int num_iter_options = iter_spec.max - iter_spec.min + 1; // Max is max index
    int iter_val = roundf(data.pots[0] * (num_iter_options - 1)) + iter_spec.min;
    iter_val = fmaxf(iter_spec.min, fminf(iter_val, iter_spec.max));
    NT_setParameterFromUi(algIdx, kParamIterations + paramOffset, iter_val);
  }
  if (data.controls & kNT_potC)
  { // Zoom
    const _NT_parameter &zoom_spec = parameters[kParamZoom];
    int zoom_val = roundf(zoom_spec.min + data.pots[1] * (zoom_spec.max - zoom_spec.min));
    zoom_val = fmaxf(zoom_spec.min, fminf(zoom_val, zoom_spec.max));
    NT_setParameterFromUi(algIdx, kParamZoom + paramOffset, zoom_val);
  }
  if (data.controls & kNT_potR)
  { // Ball Speed
    const _NT_parameter &speed_spec = parameters[kParamBallSpeed];
    int speed_val = roundf(speed_spec.min + data.pots[2] * (speed_spec.max - speed_spec.min));
    speed_val = fmaxf(speed_spec.min, fminf(speed_val, speed_spec.max));
    NT_setParameterFromUi(algIdx, kParamBallSpeed + paramOffset, speed_val);
  }

  // Encoders (L, R) -> Parameters (Offset X, Offset Y)
  if (data.encoders[0] != 0)
  { // Offset X (Encoder L)
    const _NT_parameter &offset_spec = parameters[kParamOffsetX];
    int current_val = self->v[kParamOffsetX];
    int new_val = current_val + data.encoders[0];
    new_val = fmaxf(offset_spec.min, fminf(new_val, offset_spec.max));
    NT_setParameterFromUi(algIdx, kParamOffsetX + paramOffset, new_val);
  }
  if (data.encoders[1] != 0)
  { // Offset Y (Encoder R)
    const _NT_parameter &offset_spec = parameters[kParamOffsetY];
    int current_val = self->v[kParamOffsetY];
    int new_val = current_val + data.encoders[1];
    new_val = fmaxf(offset_spec.min, fminf(new_val, offset_spec.max));
    NT_setParameterFromUi(algIdx, kParamOffsetY + paramOffset, new_val);
  }

  // Encoder Push R: Reset Ball
  if ((data.controls & kNT_encoderButtonR) && !(data.lastButtons & kNT_encoderButtonR))
  {
    // self->resetBallPriv(self->v[kParamBallSpeed]); // Old direct call
    NT_setParameterFromUi(algIdx, kParamResetTrigger + paramOffset, 1); // Set parameter to Trigger
  }
}

// --- Factory ---
_NT_factory factory = {
    .guid = NT_MULTICHAR('M', 'J', 'L', 'A'), // ModJuliaLuaAdapted
    .name = "Mod Julia",
    .description = "Modulation from ball in Julia set",
    .numSpecifications = 0,
    .specifications = NULL,
    .calculateStaticRequirements = NULL,
    .initialise = NULL,
    .calculateRequirements = calculateRequirements,
    .construct = construct,
    .parameterChanged = parameterChanged,
    .step = step,
    .draw = draw,
    .midiRealtime = NULL,
    .midiMessage = NULL,
    .tags = (uint32_t)(kNT_tagUtility | kNT_tagEffect), // Example tags
    .hasCustomUi = hasCustomUi,
    .customUi = customUi,
    .setupUi = setupUi,
};

// --- Plugin Entry Point ---
uintptr_t pluginEntry(_NT_selector selector, uint32_t data)
{
  switch (selector)
  {
  case kNT_selector_version:
    return kNT_apiVersionCurrent;
  case kNT_selector_numFactories:
    return 1;
  case kNT_selector_factoryInfo:
    return (uintptr_t)((data == 0) ? &factory : NULL);
  }
  return 0;
}