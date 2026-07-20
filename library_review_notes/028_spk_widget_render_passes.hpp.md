# Review 028 — `includes/structures/widget/rendering/spk_widget_render_passes.hpp`

- File: [includes/structures/widget/rendering/spk_widget_render_passes.hpp](<../includes/structures/widget/rendering/spk_widget_render_passes.hpp>)
- Done [X]
- Remarks and requested changes:

In this class, we should not only find the std::string for the overlay render pass "name", but we should fuse it with the render priority, with something like a spk::RenderPass::Description or spk::RenderPass::Configuration, that will contain a string and a priority, storing both value at the same place. Cause right now, we could link the wrong priority to the render passes