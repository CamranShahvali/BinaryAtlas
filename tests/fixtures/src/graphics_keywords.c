static const char* kRenderString = "DirectX Present hook candidate";
static const char* kVrString = "OpenXR runtime integration";
static const char* kEngineString = "UnityPlayer subsystem";

__attribute__((noinline)) int render_present_hook(void)
{
  volatile const char* render = kRenderString;
  volatile const char* vr = kVrString;
  volatile const char* engine = kEngineString;
  return render[0] + vr[0] + engine[0];
}

int main(void)
{
  return render_present_hook();
}
