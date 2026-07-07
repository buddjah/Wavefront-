// Outil de dev : instancie le plugin et son éditeur, rend l'UI dans une image
// PNG (sans écran) pour vérifier la mise en page / le rendu du Path Editor.
//
// Usage : wavefront_render_ui [fichier_sortie.png]

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI guiInit;

    wavefront::WavefrontAudioProcessor proc;
    proc.prepareToPlay (48000.0, 512);

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    if (editor == nullptr)
    {
        std::fprintf (stderr, "Editeur nul.\n");
        return 1;
    }

    const int w = 940, h = 640;
    editor->setSize (w, h);

    juce::Image image (juce::Image::ARGB, w, h, true);
    {
        juce::Graphics g (image);
        editor->paintEntireComponent (g, false);
    }

    const juce::File out (argc > 1 ? juce::String (argv[1])
                                   : juce::File::getCurrentWorkingDirectory().getChildFile ("ui_preview.png").getFullPathName());
    out.deleteFile();
    if (auto stream = out.createOutputStream())
    {
        juce::PNGImageFormat png;
        png.writeImageToStream (image, *stream);
        std::printf ("UI rendue -> %s\n", out.getFullPathName().toRawUTF8());
    }

    return 0;
}
