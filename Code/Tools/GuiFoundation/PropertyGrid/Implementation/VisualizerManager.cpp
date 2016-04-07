#include <GuiFoundation/PCH.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <ToolsFoundation/Document/Document.h>

EZ_IMPLEMENT_SINGLETON(ezVisualizerManager);

EZ_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, VisualizerManager)

  ON_CORE_STARTUP
  {
    EZ_DEFAULT_NEW(ezVisualizerManager);
  }

  ON_CORE_SHUTDOWN
  {
    if (ezVisualizerManager::GetSingleton())
    {
      auto ptr = ezVisualizerManager::GetSingleton();
      EZ_DEFAULT_DELETE(ptr);
    }
  }

EZ_END_SUBSYSTEM_DECLARATION

ezVisualizerManager::ezVisualizerManager()
  : m_SingletonRegistrar(this)
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezVisualizerManager::DocumentManagerEventHandler, this));
}

ezVisualizerManager::~ezVisualizerManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezVisualizerManager::DocumentManagerEventHandler, this));
}

void ezVisualizerManager::ClearActiveVisualizers(const ezDocument* pDoc)
{
  ezDeque<const ezDocumentObject*> sel;

  ezVisualizerManagerEvent e;
  e.m_pSelection = &sel;
  e.m_pDocument = pDoc;

  m_Events.Broadcast(e);
}

void ezVisualizerManager::SelectionEventHandler(const ezSelectionManagerEvent& event)
{
  const auto& sel = event.m_pDocument->GetSelectionManager()->GetSelection();

  ezVisualizerManagerEvent e;
  e.m_pSelection = &sel;
  e.m_pDocument = event.m_pDocument;

  m_Events.Broadcast(e);
}

void ezVisualizerManager::DocumentManagerEventHandler(const ezDocumentManager::Event& e)
{
  if (e.m_Type == ezDocumentManager::Event::Type::DocumentOpened)
  {
    e.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezVisualizerManager::SelectionEventHandler, this));
  }

  if (e.m_Type == ezDocumentManager::Event::Type::DocumentClosing)
  {
    ClearActiveVisualizers(e.m_pDocument);
  }

}
