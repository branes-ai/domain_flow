import torch
import torch.nn as nn
import torch.nn.functional as F

# Define the model
class ConvMLP(nn.Module):
    def __init__(self, in_channels=3, out_channels=64, kernel_size=3, mlp_hidden=128, output_size=10):
        super(ConvMLP, self).__init__()
        # Conv2D layer
        self.conv = nn.Conv2d(in_channels=in_channels, 
                            out_channels=out_channels, 
                            kernel_size=kernel_size, 
                            padding=1)
        
        # Calculate the size after convolution
        self.out_channels = out_channels
        
        # MLP layers
        self.fc1 = nn.Linear(out_channels * 28 * 28, mlp_hidden)  # Assuming 28x28 input image
        self.fc2 = nn.Linear(mlp_hidden, output_size)
        
    def forward(self, x):
        # Apply Conv2D
        x = self.conv(x)
        x = F.relu(x)
        
        # Flatten the output
        x = x.view(x.size(0), -1)
        
        # MLP layers
        x = F.relu(self.fc1(x))
        x = self.fc2(x)
        
        return x

# Example usage
def main():
    # Create model instance
    model = ConvMLP()
    
    # Create a sample input (batch_size=1, channels=3, height=28, width=28)
    sample_input = torch.randn(1, 3, 28, 28)
    
    # Forward pass
    output = model(sample_input)
    
    print(f"Input shape: {sample_input.shape}")
    print(f"Output shape: {output.shape}")
    print(f"Model architecture:\n{model}")

if __name__ == "__main__":
    main() 
