namespace KRV {

// The number of element is crusial, for each element must be placed corresponding `.obj` and `.png` files.
constexpr VkTransformMatrixKHR const blasTransformMatrices[] = {
    {
        0.5F, 0.0F, 0.0F, 0.0F,
        0.0F, 0.0F, -0.5F, -1.0F,
        0.0F, 0.5F, 0.0F, 0.0F
    },
    {
        0.1F, 0.0F, 0.0F, 0.1F,
        0.0F, 0.0F, -0.1F, -0.2F,
        0.0F, 0.1F, 0.0F, 0.0F
    }
};

}
