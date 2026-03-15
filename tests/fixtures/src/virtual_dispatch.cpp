struct Renderer
{
  virtual ~Renderer() = default;
  virtual int draw(int value) = 0;
};

struct OpenGLRenderer final : Renderer
{
  __attribute__((noinline)) int draw(int value) override
  {
    return value + 5;
  }
};

__attribute__((noinline)) int invoke(Renderer* renderer, int value)
{
  return renderer->draw(value);
}

int main()
{
  OpenGLRenderer renderer;
  return invoke(&renderer, 37);
}
