interface DeferredPageProps {
  title: string;
  message: string;
}

export function DeferredPage({
  title,
  message,
}: DeferredPageProps): React.JSX.Element {
  return (
    <section>
      <h2>{title}</h2>
      <p>{message}</p>
    </section>
  );
}
