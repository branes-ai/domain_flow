import torch
import torch.nn as nn

# Define the single-layer MLP
class SingleLayerMLP(nn.Module):
    def __init__(self, input_size=256*256, output_size=16):
        super(SingleLayerMLP, self).__init__()
        # Single linear layer
        self.linear = nn.Linear(input_size, output_size)
        
    def forward(self, x):
        # Flatten the input (batch_size, 256, 256) -> (batch_size, 256*256)
        x = x.view(x.size(0), -1)
        # Apply matmul + bias (linear layer)
        x = self.linear(x)
        return x

# Example usage
def main():
    # Create model instance
    model = SingleLayerMLP()
    
    # Create a sample batched input (batch_size=4, height=256, width=256)
    sample_input = torch.randn(4, 256, 256)
    
    # Forward pass
    output = model(sample_input)
    
    print(f"Input shape: {sample_input.shape}")
    print(f"Output shape: {output.shape}")
    print(f"Model architecture:\n{model}")

if __name__ == "__main__":
    main()