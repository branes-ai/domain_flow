import torch
import torch.nn as nn

class SingleLayerMLP(nn.Module):
    def __init__(self, input_size=256*256, output_size=16):
        super(SingleLayerMLP, self).__init__()
        self.linear = nn.Linear(input_size, output_size)
        
    def forward(self, x):
        x = x.view(x.size(0), -1)
        x = self.linear(x)
        x = nn.functional.softmax(x, dim=1)  # SoftMax across the 16 categories
        return x

def main():
    model = SingleLayerMLP()
    sample_input = torch.randn(4, 256, 256)
    output = model(sample_input)
    print(f"Input shape: {sample_input.shape}")
    print(f"Output shape: {output.shape}")
    print(f"Sum of probabilities (per sample): {output.sum(dim=1)}")  # Should be ~1
    print(f"Model architecture:\n{model}")

if __name__ == "__main__":
    main()


# In the context of the single-layer MLP you requested (mapping a 256x256 tensor to a 16-category output vector), replacing a ReLU activation with a SoftMax depends on the task and desired output interpretation:
#
# - ReLU: If you were to add a ReLU (though your provided code doesn't currently include one), it would introduce non-linearity by outputting zero for negative values and keeping positive values unchanged. This is useful for hidden layers or when you want unbounded, non-negative outputs. However, ReLU doesn't normalize outputs, so it’s not ideal for producing probabilities or ensuring outputs sum to 1.
# - SoftMax: SoftMax normalizes the output vector such that all values are positive and sum to 1, making it suitable for multi-class classification where the outputs represent probabilities across the 16 categories. This is typically used in the final layer when you need a probability distribution over mutually exclusive classes (e.g., classifying an input into one of 16 categories).
#
# Recommendation
# Since your MLP outputs a 16-category vector, SoftMax is likely more appropriate if the goal is classification, as it provides a clear probabilistic interpretation (e.g., "80% chance of class 3"). ReLU would be better suited if the output represents unbounded quantities or if this were a hidden layer feeding into further processing. If you intend to use the model for classification with cross-entropy loss (common for multi-class tasks), SoftMax is standard, though note that PyTorch’s CrossEntropyLoss combines log-SoftMax and the loss computation, so you often skip explicit SoftMax in the model.